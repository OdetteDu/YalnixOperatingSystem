#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern int SetKernelBrk(void *addr)
{
	return 0;
}

extern void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{

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




