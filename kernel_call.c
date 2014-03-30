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
int isTerminalBusy[NUM_TERMINALS];

struct queue *delayQueueHead;
struct queue *delayQueueTail;

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
	return 0;
}

extern int KernelBrk(void *addr)
{
	TracePrintf(256, "Brk: addr(%d)\n", addr);
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
}

