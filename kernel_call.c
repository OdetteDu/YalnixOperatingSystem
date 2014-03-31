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
extern struct PCBNode* active_process;
extern struct queue* waitingQHead, *waitingQTail;
extern struct queue* readyQHead, *readyQTail;
//extern struct pte** forkTBL;
extern struct pte *UserPageTable, *KernelPageTable;



extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame);
static int current_pfn;
int cur_index;
extern int KernelFork(void)
{
	TracePrintf(256, "Fork\n");
	printf("[KernelFork] entrance\n");
	struct PCBNode cur_active;
	memcpy(&cur_active, active_process, sizeof(struct PCBNode));
	struct PCBNode* newproc;
	
	//if(newproc == NULL) return ERROR;
	int offset = 0;
	newproc = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	
	if(newproc == NULL){ return ERROR;}
	
	newproc -> PID = nextPID();
	/* Find a new page table */
//	TracePrintf(100, "[Kernel Fork] try to allocate a new page table.\n");
//	struct pte *temp = KernelPageTable + VMEM_1_SIZE/PAGESIZE -2-cur_index;
//	temp->valid = 1;
//	temp->uprot = 0;
//	temp->kprot = PROT_READ | PROT_WRITE;//set access for the kernel
//	if(current_pfn == 0){//we don't have a pfn existing that we can use the rest half
//	  temp->pfn = allocatePhysicalPage();
//	  current_pfn = temp->pfn;
//	  newproc->pageTable = (struct pte*)((temp->pfn)*PAGESIZE);
//	}else{//we have an existing pfn, we can use the upper half
//	  temp->pfn = current_pfn;
//	  offset = PAGE_TABLE_LEN;//offset
//	  newproc->pageTable = (struct pte*)((temp->pfn)*PAGESIZE + PAGE_TABLE_SIZE);
//	 // cur_index ++;
//	}
//	//map the table
//	struct pte* entry = (struct pte *)(VMEM_1_LIMIT-(2+cur_index)*PAGESIZE);
//	int i;
//	for(i=0; i<VMEM_0_SIZE/PAGESIZE; i++){
//	  // struct pte new_entry;
//	  if((i<<PAGESHIFT)>=KERNEL_STACK_BASE){//this is kernel stack pages
//	    ((struct pte*)(entry+i+offset))->valid = 1;
//	    ((struct pte*)(entry+i+offset))->uprot = 0;
//	    ((struct pte*)(entry+i+offset))->kprot = PROT_READ | PROT_WRITE;
//	   
//	  }else{
//	    ((struct pte*)(entry+i+offset))->valid = 0;
//	  }
//	}
//	if(offset==PAGE_TABLE_LEN){
//		cur_index++;
//	}
//
	TracePrintf(100, "[Kernel Fork]: Try to allocate a new page table\n");
	int vpn = 511;
	struct pte *kernelPTE = &KernelPageTable[vpn];
	kernelPTE -> valid = 1;
	kernelPTE -> uprot = PROT_NONE;
	kernelPTE -> kprot = PROT_READ | PROT_WRITE;
	kernelPTE -> pfn = allocatePhysicalPage();
	printKernelPageTable(100);

	struct pte *newUserPageTable;
	long newPageTableAddr = 1023 << PAGESHIFT;
	newUserPageTable = newPageTableAddr;
	TracePrintf(100, "[Kernel Fork]: New Page Table: %d %d\n", newPageTableAddr, newUserPageTable); 
	memcpy(newUserPageTable, active_process -> pageTable, sizeof(struct pte) * PAGE_TABLE_LEN);
	newproc -> pageTable = newUserPageTable;
	//will need to change this thing later
	//	newproc -> pageTable = //forkTBL[0];//malloc(PAGE_TABLE_LEN * sizeof(struct pte));
	newproc -> status = READY;
	newproc -> blockedReason = 0;
	newproc -> numTicksRemainForDelay = 0;
	newproc -> parent = NULL;
	newproc -> child = NULL;
	newproc -> prevSibling = NULL;
	newproc -> nextSibling = NULL;
	TracePrintf(100, "[Kernel Fork]: New PCB: PID(%d), PageTable(%d)\n", newproc -> PID, newproc -> pageTable); 

	//printf("[KernelFork] context switching\n");
	TracePrintf(100, "Fork enter context switch\n");
	ContextSwitch(forkSwitchFunc, &(cur_active.ctxp), &cur_active, newproc);
	TracePrintf(100, "Fork left context switch\n");

	printf("[KernelFork] context switched\n");
	if(cur_active.PID == active_process->PID)
	{
		printf("[KernelFork] parent return\n");
		//return from parent
		return newproc->PID;
	}
	else
	{
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

	if (status !=0)
	{
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

	struct queue* prev = NULL;//need to keep the chain working
	struct queue* child = active_process->children;

	while(child!=0)
	{
		struct PCBNode* tempchild = child->proc;
		if(tempchild->status == TERMINATED)
		{
			//reap zombie child
			//need more work here to keep the chain dude
			if(prev==NULL)
			{/**********NOT SURE WHAT THE HELL I M DOING HERE **********/
				prev = child;
				child = child->next;
				free(tempchild);
				free(prev);
				prev = NULL;
			}
			else
			{
				prev->next = child->next;
				free(tempchild);
				free(child);
				child = prev->next;
			}
		}
		else
		{
			prev = child;
			tempchild->parent = 0;
			child = child->next;
		}
		// child = child->next;
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
		struct exitStatusQ* newStatus = malloc(sizeof(struct exitStatusQ));
		newStatus->PID = active_process->PID;
		newStatus->exitStatus = status;
		newStatus->next = NULL;
		struct exitStatusQ* statusQ =(active_process->parent)->exitStatusQ ;
		//put status in parent
		if(statusQ ==NULL)
		{//simply initialize the statusQ
			(active_process->parent)->exitStatusQ = newStatus;
		}
		else
		{//put the new item in the end of the status q
			while(statusQ->next != NULL)
			{//get to the end of Q
				statusQ = statusQ->next;
			}
			statusQ->next = newStatus;
		}
		TracePrintf(99, "[Kernel Exit] Now we finished recording child status we could free it.\n");
		//struct PCBNode*
		//check the waiting queue to find parent
		struct queue* newWaitQ;
		struct queue* waitingQ = waitingQHead;
		while(waitingQ!=0){//go through waiting q to resume parent
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

		/* struct PCBNode* nextReady;
       if(readyQHead==0){ nextReady = idle;}
       else{ nextReady = popQHead(readyQHead);}
       ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, nextReady);*/

	}

	return 0;
}

extern int KernelWait(int *status_ptr)
{
	int pid;
	if(status_ptr==NULL)
	{//no pointer
		return (ERROR);
	}
	//we should also check the validity of the memory in the pointer
	//we should also check the uprot
	//TODO all the things above
	TracePrintf(256, "Wait: status_ptr(%d)\n", *status_ptr);
	struct queue* children = active_process->children;
	if(children == NULL)
	{//no child for parent
		return ERROR;
	}

	if(active_process->exitStatusQ == NULL)
	{
		//simply dequeue ready queue and context switch
		struct PCBNode * nextProc;
		if(readyQHead == NULL)
		{
			nextProc = idle;
		}
		else
		{
			struct queue* temp = readyQHead;
			nextProc = readyQHead->proc;
			readyQHead = readyQHead->next;
			free(temp);//need to free the poped item from  queue
		}

		ContextSwitch(delaySwitchFunc, &(active_process->ctxp), active_process, nextProc);
	}

	TracePrintf(99, "[KernelWait] Came back from waiting \n");

	pid = (active_process->exitStatusQ)->PID;//collect the child's pid
	*(status_ptr) = (active_process->exitStatusQ)->exitStatus;
	struct exitStatusQ* temp = active_process->exitStatusQ;
	active_process->exitStatusQ = (active_process->exitStatusQ)->next;
	free(temp);//free the status pointer

	return pid;
}

extern int KernelGetPid(void)
{
	TracePrintf(256, "GetPid\n");
	return active_process->PID;
}

extern int KernelBrk(void *addr)
{
	TracePrintf(256, "Brk: addr(%d)\n", addr);

	//check if the addr is illegal
	if(addr < MEM_INVALID_SIZE)
	{
		TracePrintf(0, "Error in KernelBrk: Trying to set the brk below MEM_INVALID_SIZE.\n");
		//Exit the program.
		return ERROR;
	}

	if(addr > DOWN_TO_PAGE(active_process -> stack_brk) - PAGESIZE)
	{
		TracePrintf(0, "Error in KernelBrk: Trying to set the brk inside or above the red zone.\n");
		return ERROR;
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
			UserPageTable[i].uprot = PROT_READ | PROT_WRITE; 
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
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	}

	active_process -> heap_brk = addr;
	return 0;
}


extern int KernelDelay(int clock_ticks)
{
	TracePrintf(256, "Delay: clock_ticks(%d)\n", clock_ticks);
	if(clock_ticks < 0)
	{
		return ERROR;
	}
	else if(clock_ticks > 0)
	{
		//first set the CurPCB delay
		active_process->numTicksRemainForDelay= clock_ticks;
		active_process->status = BLOCKED;

		struct PCBNode * nextProc;
		if(readyQHead == NULL)
		{
			nextProc = idle;
		}
		else
		{
			struct queue* temp = readyQHead;
			nextProc = readyQHead->proc;
			readyQHead = readyQHead->next;
			free(temp);//need to free the poped item from  queue
		}

		ContextSwitch(delaySwitchFunc, &(active_process->ctxp), active_process, nextProc);
	}
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
//	struct TTYQueue *newQueueNode = malloc(sizeof(struct TTYQueue));
//	newQueueNode -> proc = active_process;
//	newQueueNode -> next = NULL;
//	newQueueNode -> length = len;
//	newQueueNode -> buffer = buf;//not sure about the copy here
//
//	if (isTerminalBusy[tty_id] == 0)
//	{
//		TracePrintf(200, "[KernelTtyWrite] Terminal %d is not busy, prepare to write to termianl.\n", tty_id);
//		isTerminalBusy[tty_id] = 1;
//		TTYWriteQueueHead = newQueueNode;
//		TTYWriteQueueTail = newQueueNode;
//		TtyTransmit(tty_id, TTYWriteQueueHead -> buffer, len);
//	}
//	else
//	{
//		TracePrintf(200, "[KernelTtyWrite] Terminal %d is busy, put into the blocking queue.\n", tty_id);
//		//Save the buffer and length in kernel
//		addToTTYQEnd(newQueueNode, TTYWriteQueueTail);
//	}
//	return 0;
}

