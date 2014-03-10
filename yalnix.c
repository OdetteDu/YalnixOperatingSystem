#include <comp421/yalnix.h>
#include <comp421/hardware.h>

void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapClock(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapIllegal(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
    
}

void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
    
}

extern int SetKernelBrk(void *addr)
{
	return 0;
}

extern void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{
	interruptTable[TRAP_KERNEL] = trapKernel;
	interruptTable[TRAP_CLOCK] = trapClock;
	interruptTable[TRAP_ILLEGAL] = trapIllegal;
	interruptTable[TRAP_MEMORY] = trapMemory;
	interruptTable[TRAP_MATH] = trapMath;
	interruptTable[TRAP_TTY_RECEIVE] = trapTTYReceive;
	interruptTable[TRAP_TTY_TRANSMIT] = trapTTYTransmit;
	RCS421RegVal interruptTableAddress = (RCS421RegVal)interruptTable;
	WriteRegister(REG_VECTOR_BASE, interruptTableAddress);

}

extern int Fork(void)
{
	return 0;
}

extern int Exec(char *filename, char **argvec)
{
	return 0;
}

extern void Exit(int status)
{
	
}

extern int Wait(int *status_ptr)
{
	return 0;
}

extern int GetPid(void)
{
	return 0;
}

extern int Brk(void *addr)
{
	return 0;
}

extern int Delay(int clock_ticks)
{
	return 0;
}

extern int TtyRead(int tty_id, void *buf, int len)
{
	return 0;
}

extern int TtyWrite(int tty_id, void *buf, int len)
{
	return 0;
}




