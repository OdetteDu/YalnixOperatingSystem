#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern int SetKernelBrk(void *addr)
{

}

extern void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{

}

extern int Fork(void)
{

}

extern int Exec(char *filename, char **argvec)
{

}

extern void Exit(int status) __attribute__ ((noreturn))
{

}

extern int Wait(int *status_ptr)
{

}

extern int GetPid(void)
{

}

extern int Brk(void *addr)
{

}

extern int Delay(int clock_ticks)
{

}

extern int TtyRead(int tty_id, void *buf, int len)
{

}

extern int TtyWrite(int tty_id, void *buf, int len)
{

}




