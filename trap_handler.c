#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "global.h"
#include "util.h"
#include "kernel_call.h"
#include "trap_handler.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
unsigned int clockTick;
extern struct PCBNode* init;
extern struct PCBNode* idle;
extern struct PCBNode* active_process;

extern struct queue *waitingQHead, *waitingQTail;
extern struct queue *readyQHead, *readyQTail;
extern struct queue *delayQueueHead, *delayQueueTail;

extern void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapKernel: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
			exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
			exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
			exceptionStackFrame->regs);

	//	int temp = 0;
	switch(exceptionStackFrame->code)
	{
	case YALNIX_FORK:
		exceptionStackFrame->regs[0] = KernelFork();
		break;
	case YALNIX_EXEC:
		exceptionStackFrame->regs[0] = KernelExec((char *)exceptionStackFrame->regs[1], (char **)exceptionStackFrame->regs[2], exceptionStackFrame);
		break;
	case YALNIX_EXIT:
		KernelExit((int)exceptionStackFrame->regs[1]);
		break;
	case YALNIX_WAIT:
		exceptionStackFrame->regs[0] = KernelWait((int*)exceptionStackFrame->regs[1]);
		break;
	case YALNIX_GETPID:
		exceptionStackFrame -> regs[0] = KernelGetPid();
		break;
	case YALNIX_BRK:
	  exceptionStackFrame -> regs[0] = KernelBrk((void*)exceptionStackFrame -> regs[1]);
		break;;
	case YALNIX_DELAY:
		exceptionStackFrame->regs[0] = KernelDelay((int)exceptionStackFrame->regs[1]);
		break;
	case YALNIX_TTY_READ:
		break;
	case YALNIX_TTY_WRITE:
		break;
	default:
		break;
	}
}

extern void trapClock(ExceptionStackFrame *exceptionStackFrame)
{

	TracePrintf(512, "trapClock: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
			exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
			exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
			exceptionStackFrame->regs);

	struct queue* previous = NULL;
	struct queue *current = delayQueueHead;
	while(current != NULL)
	{
		struct PCBNode *currentPCB = current -> proc;
		(currentPCB -> numTicksRemainForDelay) --;
		TracePrintf(80, "[Trap Clock Delay]: PID(%d), numTicksRemainForDelay(%d)\n", currentPCB -> PID, currentPCB -> numTicksRemainForDelay);
		if(currentPCB -> numTicksRemainForDelay == 0)
		{
			currentPCB->status = READY;
			//remove that specific PCB from delayQueue
			//addToQEnd(currentPCB, readyQueueTail);
			if(previous==NULL)
			{//when current is the head of the list
				previous = current;
				delayQueueHead = current->next;
				//push to readyQ
				if(readyQHead == NULL)
				{
					readyQHead = previous;
					readyQTail = previous;
				}
				else
				{
					readyQTail->next = previous;
					readyQTail = previous;
				}//pushed
				previous = NULL;
				current = delayQueueHead;

			}
			else
			{//when current is not head of things
				previous->next = current->next;
				//push to readyQ
				if(readyQHead == NULL)
				{
					readyQHead = current;
					readyQTail = current;
				}
				else
				{
					readyQTail->next = current;
					readyQTail = current;
				}//pushed
				current = previous->next;
			}//end-of-remove-from-delay-queue
		}
		else
		{
			previous = current;
			current = current->next;
		}
	}

	if(clockTick >= 1)
	{

		if(readyQHead != NULL)
		{
		  //	printf("We are poping ready Q head right now!\n");
			struct queue* head = readyQHead;
			struct PCBNode* p2 = head->proc;
			readyQHead = head->next;
			free(head);

			//	printf("We poped out PID %d\n", p2->PID);
			if(readyQHead == NULL) readyQTail = NULL;
			ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, p2);
			//	printf("Now the ready queue head is%d\n", (readyQHead->proc)->PID);
			clockTick = 0;
			TracePrintf(256, "Trap_clock: switch from pid1: %d to pid2: %d\n", active_process->PID, p2->PID);
		}
		else
		{
			if(active_process!= idle)
			{
			  //	printf("We are just switching to idle!\n");
				ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, idle);
				clockTick = 0;
				TracePrintf(256, "Trap_clock: switch from pid1: %d to pid2: %d\n", active_process->PID, idle->PID);
			}//otherwise do not process

		}
		clockTick = 0;
	}
	else
	{
		clockTick ++;
	}
}

extern void trapIllegal(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapIllegal: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
			exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
			exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
			exceptionStackFrame->regs);

	int code = exceptionStackFrame->code;
	char string[128];
	char* msg = string;

	if (code == ILL_BADSTK)
		msg = "Bad stack\n";
	else if (code == ILL_ILLOPC || code == ILL_ILLOPN || code == ILL_ILLADR)
		msg = "Illegal instruction\n";
	else if (code == ILL_PRVOPC || code == ILL_PRVREG)
		msg = "Privileged instruction\n";
	else if (code == ILL_COPROC)
		msg = "Coprocessor error\n";
	else if (code == ILL_ILLTRP)
		msg = "Illegal software trap\n";
	else if (code == BUS_ADRALN + 20)
		printf(msg, "Invalid address alignment %p", exceptionStackFrame->addr);
	else if (code == SI_KERNEL)
		msg = "Linux kernel SIGILL\n";
	else if (code == SI_USER)
		msg = "Received SIGILL from user\n";
	else
		printf(msg, "Unknown code 0x%x", code);

	KernelExit(ERROR);
}

extern void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapMemory: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
			exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
			exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
			exceptionStackFrame->regs);

	if( (uint64_t)(exceptionStackFrame -> addr) > (active_process -> stack_brk) )
	{
		TracePrintf(0, "Trap Memory Error: PID: %d, addr: %d is large than stack_brk: %d\n", active_process -> PID, exceptionStackFrame -> addr, active_process -> stack_brk);
		KernelExit(ERROR);
	}

	if( (uint64_t)(exceptionStackFrame -> addr) < (UP_TO_PAGE( active_process -> heap_brk) + PAGESIZE) )
	{
		TracePrintf(0, "Trap Memory Error: PID: %d, addr: %d is smaller than heap_brk: %d\n", active_process -> PID, exceptionStackFrame -> addr, active_process -> heap_brk);;
		KernelExit(ERROR);
	}

	long userTablePTE;
	TracePrintf(510, "Moving user stack down to address: %d (%d)\n", exceptionStackFrame -> addr, (long)exceptionStackFrame -> addr >> PAGESHIFT);
	for (userTablePTE = (DOWN_TO_PAGE(exceptionStackFrame -> addr)); userTablePTE <= (DOWN_TO_PAGE(active_process -> stack_brk)); userTablePTE += PAGESIZE)
	{
		unsigned int i = ((userTablePTE) >> PAGESHIFT) % PAGE_TABLE_LEN;
		UserPageTable[i].valid = 1;
		UserPageTable[i].uprot = PROT_READ | PROT_WRITE;
		UserPageTable[i].kprot = PROT_READ | PROT_WRITE;
		/* Need to change the pfn here */
		//need to check if there are enough physical pages
		UserPageTable[i].pfn = allocatePhysicalPage();
		TracePrintf(250, "Allocate physical pages for user process: PID(%d), VPN(%d), PFN(%d).\n", active_process -> PID, i, UserPageTable[i].pfn);
	}
}

extern void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapMath: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
			exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
			exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
			exceptionStackFrame->regs);
	char string[128];
	char *msg = string;
	switch (exceptionStackFrame->code)
	{
	case FPE_INTOVF:
		msg = "Integer overflow";
		break;
	case FPE_INTDIV:
		msg = "Integer divide by zero";
		break;
	case FPE_FLTRES:
		msg = "Floating inexact result";
		break;
	case FPE_FLTDIV:
		msg = "Floating divide by zero";
		break;
	case FPE_FLTUND:
		msg = "Floating underflow";
		break;
	case FPE_FLTINV:
		msg = "Invalid floating operation";
		break;
	case FPE_FLTSUB:
		msg = "FP subscript out of range";
		break;
	case FPE_FLTOVF:
		msg = "Floating overflow";
		break;
	case SI_KERNEL:
		msg = "Linux kernel SIGFPE";
		break;
	case SI_USER:
		msg = "Received SIGFPE from user";
		break;
	default:
		printf(msg, "Unknown code %d", exceptionStackFrame->code);
	}

	KernelExit(ERROR);
}

extern void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYReceive: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

extern void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYTransmit: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
	//how to put the buffer back to the buffer the process asked?
	//addToQEnd(popTTYQHead(TTYWriteQueueHead), readyQueueTail);
	//How to know which terminal is this?

	//	if(TTYWriteQueueHead != NULL)
	//	{
	//		TtyTransmit(tty_id, TTYWriteQueueHead -> buffer, TTYWriteQueueHead -> length);
	//	}
	//	else
	//	{
	//		isTerminalBusy[tty_id] = 0;
	//		TTYWriteQueueHead = NULL;
	//		TTYWriteQueueTail = NULL;
	//	}
}


