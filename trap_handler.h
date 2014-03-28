#ifndef _trap_handler_h
#define _trap_handler_h

#include <comp421/hardware.h>
typedef void (*trap_handler)(ExceptionStackFrame *);

/* Interrupt handler declarations */
extern void trapKernel(ExceptionStackFrame *frame);
extern void trapClock(ExceptionStackFrame *frame);
extern void trapIllegal(ExceptionStackFrame *frame);
extern void trapMath(ExceptionStackFrame *frame);
extern void trapTTYReceive(ExceptionStackFrame *frame);
extern void trapTTYTransmit(ExceptionStackFrame *frame);
extern void trapMemory(ExceptionStackFrame *frame);


#endif /* end _trap_handler_h */
