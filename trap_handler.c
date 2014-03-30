#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "global.h"
#include "util.h"
#include "kernel_call.h"
#include "trap_handler.h"
#include <stdlib.h>
#include <stdio.h>

static int clockCount = 0;
struct PCBNode* init;
struct PCBNode* idle;
struct PCBNode* active_process;

extern void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
  
  TracePrintf(512, "trapKernel: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
	      exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
	      exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
	      exceptionStackFrame->regs);
  int temp = 0;
  switch(exceptionStackFrame->code){
  case YALNIX_FORK:
    TracePrintf(0, "calling kernel Fork()");
    break;
  case YALNIX_EXEC:
    KernelExec((char *)exceptionStackFrame->regs[1], (char **)exceptionStackFrame->regs[2], exceptionStackFrame);
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
  
  if(active_process->PID == 0)
    {
      clockCount = 1;
      //	//TracePrintf(510, "Waiting for the next trap clock to do context switch\n");
      ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process,init); 
      TracePrintf(510, "Trap_clock: switch from idle to init\n");
    }
  else
    {
      clockCount = 0;
      ContextSwitch(generalSwitchFunc, &(active_process->ctxp), active_process, idle);
      TracePrintf(510, "Trap_clock: switch from init to idle\n");
	    
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
  
  //KernelExit(ERROR);
  printf(msg);
}

extern void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
  TracePrintf(512, "trapMemory: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
	      exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
	      exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
	      exceptionStackFrame->regs);
    
}

extern void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
    
  TracePrintf(512, "trapMath: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n",
	      exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr,
	      exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,
	      exceptionStackFrame->regs);
  char string[128];
  char *msg = string;
  switch (exceptionStackFrame->code) {
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

  //Kernel_Exit(ERROR);
}

extern void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
  TracePrintf(512, "trapTTYReceive: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

extern void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
  TracePrintf(512, "trapTTYTransmit: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}


