#ifndef _kernel_call_h
#define _kernel_call_h

#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern int KernelFork(void);
extern int KernelExec(char *filename, char **argvec, ExceptionStackFrame *frame);
extern int KernelExit(int status);
extern int KernelWait(int *status_ptr);
extern int KernelGetPid(void);
extern int KernelBrk(void *addr);
extern int KernelDelay(int clock_ticks);
extern int KernelTtyRead(int tty_id, void *buf, int len);
extern int KernelTtyWrite(int tty_id, void *buf, int len);

#endif /* end _kernel_call_h */
