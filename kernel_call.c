#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"

extern int KernelFork(void)
{
	TracePrintf(512, "Fork\n");
	return 0;
}

extern int KernelExec(char *filename, char **argvec)
{
	TracePrintf(512, "Exec: filename(%s), argvec(%s)\n", filename, argvec);
	return 0;
}

extern int KernelExit(int status)
{
	TracePrintf(512, "Exit: status(%d)\n", status);
	return 0;
}

extern int KernelWait(int *status_ptr)
{
	TracePrintf(512, "Wait: status_ptr(%d)\n", *status_ptr);
	return 0;
}

extern int KernelGetPid(void)
{
	TracePrintf(512, "GetPid\n");
	return 0;
}

extern int KernelBrk(void *addr)
{
	TracePrintf(512, "Brk: addr(%d)\n", addr);
	return 0;
}

extern int KernelDelay(int clock_ticks)
{
	TracePrintf(512, "Delay: clock_ticks(%d)\n", clock_ticks);
	return 0;
}

extern int KernelTtyRead(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyRead: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}

extern int KernelTtyWrite(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyWrite: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}
