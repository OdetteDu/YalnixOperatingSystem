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
	TracePrintf(256, "Fork\n");
	return 0;
}

extern int KernelExec(char *filename, char **argvec)
{
	TracePrintf(256, "Exec: filename(%s), argvec(%s)\n", filename, argvec);
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

extern int KernelBrk(void *addr, struct PCBNode *pcb)
{
	TracePrintf(256, "Brk: addr(%d)\n", addr);

	//check if the addr is illegal
	if(addr < MEM_INVALID_SIZE)
	{
		  TracePrintf(0, "Error in KernelBrk: Trying to set the brk below MEM_INVALID_SIZE.\n");
		  //Exit the program.
	}

	if(addr > DOWN_TO_PAGE(pcb -> stack_brk) - PAGESIZE)
	{
		  TracePrintf(0, "Error in KernelBrk: Trying to set the brk inside or above the red zone.\n");
	}

    unsigned int userTablePTE;
    //Only allocate for entire pages??
    unsigned int gap = (UP_TO_PAGE(addr)-UP_TO_PAGE(pcb -> heap_brk));
    if(gap>0)
	{
		TracePrintf(250, "Moving user brk up to address: %d (%d)\n", addr, (long)addr >> PAGESHIFT);
		for (userTablePTE = (UP_TO_PAGE(pcb -> heap_brk)); userTablePTE < (UP_TO_PAGE(addr)); userTablePTE += PAGESIZE)
		{
			unsigned int i = ((userTablePTE) >> PAGESHIFT) % PAGE_TABLE_LEN;
			UserPageTable[i].valid = 1;
			UserPageTable[i].uprot = PROT_NONE;
			UserPageTable[i].kprot = PROT_READ | PROT_WRITE;
			/* Need to change the pfn here */
			UserPageTable[i].pfn = allocatePhysicalPage();
			TracePrintf(250, "Allocate physical pages for user process: PID(%d), VPN(%d), PFN(%d).\n", pcb -> PID, i, UserPageTable[i].pfn);
		}
	}
	else if (gap<0)
	{
		TracePrintf(250, "Moving user brk down to address: %d (%d)\n", addr, (long)addr >> PAGESHIFT);
		for ( userTablePTE = (UP_TO_PAGE(addr)); userTablePTE < (UP_TO_PAGE(pcb -> heap_brk)); userTablePTE += PAGESIZE)
		{
			unsigned int i =((userTablePTE) >> PAGESHIFT) % PAGE_TABLE_LEN;
			UserPageTable[i].valid = 0;
			freePhysicalPage(KernelPageTable[i].pfn);
		}
	}

	pcb -> heap_brk = addr;
	return 0;
}

extern int KernelDelay(int clock_ticks)
{
	TracePrintf(256, "Delay: clock_ticks(%d)\n", clock_ticks);
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
	return 0;
}

