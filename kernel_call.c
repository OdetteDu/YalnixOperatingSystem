#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include <stdio.h>

/* Var */
struct PCBNode* active_process;
struct queue* waitingQHead, *waitingQTail;
struct queue* readyQHead, *readyQTail;
struct queue *delayQueueHead, *delayQueueTail;
extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame);

/* TTY */
int isTerminalBusy[NUM_TERMINALS];
//head is also the active process who is waiting for interrupt
struct TTYQueue *TTYWriteQueueHead;
struct TTYQueue *TTYWriteQueueTail;

extern int KernelFork(void)
{
	TracePrintf(256, "Fork\n");
	printf("[KernelFork] entrance\n");
	struct PCBNode cur_active = *active_process;
	struct PCBNode* newproc = malloc(sizeof(struct PCBNode));

	if(newproc == NULL) return ERROR;

	newproc = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	newproc -> PID = nextPID(); 
	//will need to change this thing later
	newproc -> pageTable = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
	newproc -> status = READY;
	newproc -> blockedReason = 0;
	newproc -> numTicksRemainForDelay = 0;
	newproc -> parent = NULL;
	newproc -> child = NULL;
	newproc -> prevSibling = NULL;
	newproc -> nextSibling = NULL;

	printf("[KernelFork] context switching\n");
	TracePrintf(100, "Fork enter context switch\n");
	ContextSwitch(forkSwitchFunc, &(cur_active.ctxp), &cur_active, newproc);
	TracePrintf(100, "Fork left context switch\n");

	printf("[KernelFork] context switched\n");
	if(cur_active.PID == active_process->PID){
		printf("[KernelFork] parent return\n");
		//return from parent
		return newproc->PID;
	}else{
		printf("[KernelFork] child return \n");
		//this is returning from child, which means we should maintain the child queue
		return 0;
	}
	return 0;
}

extern int KernelExec(char *filename, char **argvec, ExceptionStackFrame *frame)
{
	TracePrintf(256, "Exec: filename(%s), argvec(%s)\n", filename, argvec);

	int status = LoadProgram(filename, argvec, frame);

	if (status !=0) {
		//KernelExit here
		return ERROR;
	}
	else
		return 0;
}

extern int KernelExit(int status)
{
	TracePrintf(256, "Exit: status(%d)\n", status);
	//	struct PCBNode* parent;
	unsigned int i;

	/* Should first free everything in region0*/
	for(i=0; i<PAGE_TABLE_LEN- KERNEL_STACK_PAGES; i++)
	{
		if(UserPageTable[i].valid == 1)
		{
			UserPageTable[i].kprot |= PROT_WRITE;
		}
	}

	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
	for(i=0; i<PAGE_TABLE_LEN; i++)
	{
		if(UserPageTable[i].valid == 1)
		{
			freePhysicalPage(UserPageTable[i].pfn);
			UserPageTable[i].valid = 0;
		}
	}
	active_process->status = TERMINATED;
	TracePrintf(0, "[Exit] freed region 0\n");
	struct queue* child = active_process->children;
	while(child!=0)
	{
		struct PCBNode* tempchild = child->proc;
		if(tempchild->status == TERMINATED)
		{
			//reap zombie child
			free(tempchild);
		}
		else
		{
			tempchild->parent = 0;
		}
		child = child->next;
	}

	TracePrintf(0, "[Exit] reaped zombie children\n");
	if(active_process->PID == 1 || active_process->PID == 0 )
	{
		Halt();
	}

	if(active_process->parent == 0)
	{
		//orphan process
		struct PCBNode* next;
		if(readyQHead == NULL)
		{
			next = idle;//when no one in ready q just switch with idle
		}
		else
		{
			next = popQHead(readyQHead);
		}
		next->status = ACTIVE;
		ContextSwitch(exitSwitchFunc, &(active_process->ctxp), active_process, next);
	}
	else
	{//has a parent
		//do something with return status
		//struct PCBNode*
		//check the waiting queue to find parent
		struct queue* newWaitQ;
		struct queue* waitingQ = waitingQHead;
		while(waitingQ!=0)
		{//go through waiting q to resume parent
			struct PCBNode* item = waitingQ->proc;
			if(active_process->parent == item)
			{
				//Unblock;
				item->status = ACTIVE;

				if(newWaitQ==0)
				{
					waitingQHead = waitingQ->next;
				}
				else
				{
					newWaitQ->next = waitingQ->next;
				}
				ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, item);
			}
		}

		struct PCBNode* nextReady;
		if(readyQHead==0)
		{
			nextReady = idle;
		}
		else
		{
			nextReady = popQHead(readyQHead);
		}
		ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, nextReady);

	}

	return 0;
}

extern int KernelWait(int *status_ptr)
{
	TracePrintf(256, "Wait: status_ptr(%d)\n", *status_ptr);
	return 0;
}

extern int KernelGetPid(void)
{
	TracePrintf(256, "GetPid\n");
	return active_process -> PID;
}

extern int KernelBrk(void *addr)
{
	TracePrintf(256, "Brk: addr(%d)\n", addr);
	TracePrintf(256, "Brk: addr(%d)\n", addr);

	//check if the addr is illegal
	if(addr < MEM_INVALID_SIZE)
	{
		TracePrintf(0, "Error in KernelBrk: Trying to set the brk below MEM_INVALID_SIZE.\n");
		//Exit the program.
	}

	if(addr > DOWN_TO_PAGE(active_process -> stack_brk) - PAGESIZE)
	{
		TracePrintf(0, "Error in KernelBrk: Trying to set the brk inside or above the red zone.\n");
	}

	unsigned int userTablePTE;
	//Only allocate for entire pages??
	unsigned int gap = (UP_TO_PAGE(addr)-UP_TO_PAGE(active_process -> heap_brk));
	if(gap>0)
	{
		TracePrintf(250, "Moving user brk up to address: %d (%d)\n", addr, (long)addr >> PAGESHIFT);
		for (userTablePTE = (UP_TO_PAGE(active_process -> heap_brk)); userTablePTE < (UP_TO_PAGE(addr)); userTablePTE += PAGESIZE)
		{
			unsigned int i = ((userTablePTE) >> PAGESHIFT) % PAGE_TABLE_LEN;
			UserPageTable[i].valid = 1;
			UserPageTable[i].uprot = PROT_NONE;
			UserPageTable[i].kprot = PROT_READ | PROT_WRITE;
			/* Need to change the pfn here */
			UserPageTable[i].pfn = allocatePhysicalPage();
			TracePrintf(250, "Allocate physical pages for user process: PID(%d), VPN(%d), PFN(%d).\n", active_process -> PID, i, UserPageTable[i].pfn);
		}
	}
	else if (gap<0)
	{
		TracePrintf(250, "Moving user brk down to address: %d (%d)\n", addr, (long)addr >> PAGESHIFT);
		for ( userTablePTE = (UP_TO_PAGE(addr)); userTablePTE < (UP_TO_PAGE(active_process -> heap_brk)); userTablePTE += PAGESIZE)
		{
			unsigned int i =((userTablePTE) >> PAGESHIFT) % PAGE_TABLE_LEN;
			UserPageTable[i].valid = 0;
			freePhysicalPage(KernelPageTable[i].pfn);
		}
	}

	active_process -> heap_brk = addr;
	return 0;
	return 0;
}

extern int KernelDelay(int clock_ticks)
{
	TracePrintf(256, "Delay: clock_ticks(%d)\n", clock_ticks);
	struct queue *newQueueNode;
	newQueueNode = malloc(sizeof(struct queue));
	active_process -> numTicksRemainForDelay = clock_ticks;
	newQueueNode -> proc = active_process;
	addToQEnd(newQueueNode, delayQueueTail);
	return 0;
}

extern int KernelTtyRead(int tty_id, void *buf, int len)
{
	TracePrintf(256, "TtyRead: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}

extern int KernelTtyWrite(int tty_id, void *buf, int len)
{
	TracePrintf(256, "TtyWrite: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	struct TTYQueue *newQueueNode = malloc(sizeof(struct TTYQueue));
	newQueueNode -> proc = active_process;
	newQueueNode -> next = NULL;
	newQueueNode -> length = len;
	newQueueNode -> buffer = buf;//not sure about the copy here

	if (isTerminalBusy[tty_id] == 0)
	{
		TracePrintf(200, "[KernelTtyWrite] Terminal %d is not busy, prepare to write to termianl.\n", tty_id);
		isTerminalBusy[tty_id] = 1;
		TTYWriteQueueHead = newQueueNode;
		TTYWriteQueueTail = newQueueNode;
		TtyTransmit(tty_id, TTYWriteQueueHead -> buffer, len);
	}
	else
	{
		TracePrintf(200, "[KernelTtyWrite] Terminal %d is busy, put into the blocking queue.\n", tty_id);
		//Save the buffer and length in kernel
		addToTTYQEnd(newQueueNode, TTYWriteQueueTail);
	}
	return 0;
	return 0;
}

