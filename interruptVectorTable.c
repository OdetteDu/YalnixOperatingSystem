#include <comp421/hardware.h>
#include <comp421/yalnix.h>

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
