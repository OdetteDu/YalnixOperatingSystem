#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame);

//interrupt vector table
void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

//Kernel brk
void *new_brk;

//Page Tables
struct pte KernelPageTable[PAGE_TABLE_LEN];
struct pte UserPageTable[PAGE_TABLE_LEN];

//Current process
int currentPID;
SavedContext *currentSavedContext;

//Available Physical Pages
struct PhysicalPageNode
{
	  int pageNumber;
	  struct PhysicalPageNode *next;
};

int numPhysicalPagesLeft;
struct PhysicalPageNode *physicalPageNodeHead;

//PCB
struct PCBNode
{
	  int PID;
	  struct pte *pageTable[PAGE_TABLE_LEN];
	  SavedContext ctxp;
	  struct PCBNode *next;
};

struct PCBNode *readyQuqueHead;
struct PCBNode *readyQueueTail;

//Allocate and free physical pages
int allocatePhysicalPage()
{
	struct PhysicalPageNode *allocatedPhysicalPageNode = physicalPageNodeHead;
	physicalPageNodeHead = allocatedPhysicalPageNode -> next;
	int number = allocatedPhysicalPageNode -> pageNumber;
	free(allocatedPhysicalPageNode);
	numPhysicalPagesLeft--;
	return number;
}

void freePhysicalPage(int pfn)
{
	struct PhysicalPageNode *newPhysicalPageNode;
	newPhysicalPageNode = (struct PhysicalPageNode *)malloc(sizeof(struct PhysicalPageNode));
	newPhysicalPageNode -> pageNumber = pfn;

	if( physicalPageNodeHead != NULL)
	{
		newPhysicalPageNode -> next = physicalPageNodeHead -> next;
		physicalPageNodeHead -> next = newPhysicalPageNode;
	}
	else
	{
		newPhysicalPageNode -> next = NULL;
		physicalPageNodeHead = newPhysicalPageNode;
	}

	numPhysicalPagesLeft++;
}

//Insert and remove from ready queue
void addFirstToReadyQueue(int pid, struct pte *pageTable[PAGE_TABLE_LEN], SavedContext ctxp)
{
	struct PCBNode *newPCBNode;
	newPCBNode = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	newPCBNode -> PID = pid;
	newPCBNode -> pageTable = pageTable;
	newPCBNode -> ctxp = ctxp;

	if( readyQuqueHead == NULL )
	{
		newPCBNode -> next = NULL;
		readyQuqueHead = newPCBNode;
		readyQueueTail = newPCBNode;
	}
	else
	{
		newPCBNode -> next = readyQuqueHead;
		readyQuqueHead = newPCBNode;
	}
}

void addLastToReadyQueue(int pid, struct pte *pageTable[PAGE_TABLE_LEN], SavedContext ctxp)
{
	struct PCBNode *newPCBNode;
	newPCBNode = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	newPCBNode -> PID = pid;
	newPCBNode -> pageTable = pageTable;
	newPCBNode -> ctxp = ctxp;

	if( readyQuqueHead == NULL )
	{
		newPCBNode -> next = NULL;
		readyQuqueHead = newPCBNode;
		readyQueueTail = newPCBNode;
	}
	else
	{
		readyQueueTail -> next = newPCBNode;
		readyQueueTail = newPCBNode;
	}
}

struct PCBNode *removeFirstFromReadyQueue()
{
	struct PCBNode *nodeTobeRemove = readyQuqueHead;
	readyQuqueHead = readyQuqueHead -> next;
	return nodeTobeRemove;
}

//Util print functions for debug
void printPhysicalPageLinkedList()
{
	TracePrintf(3072, "Free Physical Pages: \n");
	struct PhysicalPageNode *current = physicalPageNodeHead;
	while(current != NULL)
	{
		TracePrintf(3072, "%d\n", current -> pageNumber);
		current = current -> next;
	}
}

void printKernelPageTable(int level)
{
	TracePrintf(level, "Print Kernel Page Table\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		  TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, KernelPageTable[index].valid, KernelPageTable[index].pfn);
	}
}

void printUserPageTable(int level)
{
	TracePrintf(level, "Print User Page Table\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		  TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, UserPageTable[index].valid, UserPageTable[index].pfn);
	}
}

void trapKernel(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapKernel: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapClock(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapClock: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapIllegal(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapIllegal: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapMemory(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapMemory: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
    
}

void trapMath(ExceptionStackFrame *exceptionStackFrame)
{
    
	TracePrintf(512, "trapMath: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

void trapTTYReceive(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYReceive: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

void trapTTYTransmit(ExceptionStackFrame *exceptionStackFrame)
{
	TracePrintf(512, "trapTTYTransmit: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", exceptionStackFrame->vector, exceptionStackFrame->code, exceptionStackFrame->addr, exceptionStackFrame->psr, exceptionStackFrame->pc, exceptionStackFrame->sp,exceptionStackFrame->regs);
}

SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2)
{
	((struct PCBNode *)p1) -> PID = currentPID;
	((struct PCBNode *)p1) -> pageTable = &UserPageTable;
	((struct PCBNode *)p1) -> ctxp = currentSavedContext;
	currentPID = ((struct PCBNode *)p2) -> PID;
	struct pte *pt = ((struct PCBNode *)p2) -> pageTable;
	UserPageTable = &pt;
	currentSavedContext = ((struct PCBNode *)p2) -> ctxp;
	RCS421RegVal userPageTableAddress = (RCS421RegVal)UserPageTable;
	WriteRegister(REG_PTR0, userPageTableAddress);
	return currentSavedContext; 
}

extern int SetKernelBrk(void *addr)
{
	TracePrintf(512, "Set Kernel Brk Called: addr >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)addr >> PAGESHIFT, UP_TO_PAGE(addr), UP_TO_PAGE(addr) >> PAGESHIFT, DOWN_TO_PAGE(addr), DOWN_TO_PAGE(addr) >> PAGESHIFT);
	if(new_brk == NULL)
	{
		  new_brk = addr;
		  TracePrintf(1280, "Set new_brk from NULL to %d\n", new_brk);
	}
	else
	{
		  if(addr > new_brk)
		  {
				TracePrintf(1280, "Set new_brk from %d to %d\n", new_brk, addr);
				new_brk = addr;
		  }
	}
	return 0;
}

extern void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{
	int index;
	//initialize the interrupt vector Table
	for (index = 0; index < sizeof(interruptTable); index++)
	{
		interruptTable[index] = 0;
	}
	
	interruptTable[TRAP_KERNEL] = trapKernel;
	interruptTable[TRAP_CLOCK] = trapClock;
	interruptTable[TRAP_ILLEGAL] = trapIllegal;
	interruptTable[TRAP_MEMORY] = trapMemory;
	interruptTable[TRAP_MATH] = trapMath;
	interruptTable[TRAP_TTY_RECEIVE] = trapTTYReceive;
	interruptTable[TRAP_TTY_TRANSMIT] = trapTTYTransmit;
	RCS421RegVal interruptTableAddress = (RCS421RegVal)interruptTable;
	WriteRegister(REG_VECTOR_BASE, interruptTableAddress);
    
    //initialize the physical pages array
	int numOfPagesAvailable = pmem_size/PAGESIZE;
	numPhysicalPagesLeft = numOfPagesAvailable;
	TracePrintf(1024, "Total number of physical pages: %d, Available pages: %d\n", numOfPagesAvailable, numPhysicalPagesLeft);

    struct PhysicalPageNode *physicalPages[numOfPagesAvailable];
	for ( index=0; index<numOfPagesAvailable; index++)
	{
		struct PhysicalPageNode *newNode;
		newNode = (struct PhysicalPageNode *) malloc(sizeof(struct PhysicalPageNode));
		newNode -> pageNumber = index;
		newNode -> next = NULL;
		physicalPages[index] = newNode;
	}
	
	//initialize the page Table
	for( index = 0; index < PAGE_TABLE_LEN; index++ )
	{
		  struct pte PTE;
		  PTE.valid = 0;
		  PTE.pfn = 0;
		  PTE.uprot = PROT_NONE;
		  PTE.kprot = PROT_NONE;
		  KernelPageTable[index] = PTE;
	}
	TracePrintf(1024, "PAGE_TABLE_LEN: %d, KernelPageTable Size: %d\n", PAGE_TABLE_LEN, sizeof(KernelPageTable));

	for( index = 0; index < PAGE_TABLE_LEN; index++ )
	{
		  struct pte PTE;
		  PTE.valid = 0;
		  PTE.pfn = 0;
		  PTE.uprot = PROT_NONE;
		  PTE.kprot = PROT_NONE;
		  UserPageTable[index] = PTE;
	}

	//calculated the existing use of memory

	TracePrintf(1024, "PMEM_BASE: %d, VMEM_BASE: %d, VMEM_0_BASE: %d (%d), VMEM_0_LIMIT: %d (%d), VMEM_1_BASE: %d (%d), VMEM_1_LIMIT: %d (%d)\n", PMEM_BASE, VMEM_BASE, VMEM_0_BASE, VMEM_0_BASE >> PAGESHIFT, VMEM_0_LIMIT, VMEM_0_LIMIT >> PAGESHIFT, VMEM_1_BASE, VMEM_1_BASE >> PAGESHIFT, VMEM_1_LIMIT, VMEM_1_LIMIT >> PAGESHIFT);

	long etextAddr = (long)&_etext;
	TracePrintf(1024, "&_etext: %d, &_etext >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", &_etext, etextAddr >> PAGESHIFT, UP_TO_PAGE(etextAddr), UP_TO_PAGE(etextAddr) >> PAGESHIFT, DOWN_TO_PAGE(etextAddr), DOWN_TO_PAGE(etextAddr) >> PAGESHIFT);

	TracePrintf(1024, "orig_brk: %d, orig_brk >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n",orig_brk, (long)orig_brk >> PAGESHIFT, UP_TO_PAGE(orig_brk), UP_TO_PAGE(orig_brk) >> PAGESHIFT, DOWN_TO_PAGE(orig_brk), DOWN_TO_PAGE(orig_brk) >> PAGESHIFT);

	TracePrintf(1024, "new_brk: %d, new_brk >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n",new_brk, (long)new_brk >> PAGESHIFT, UP_TO_PAGE(new_brk), UP_TO_PAGE(new_brk) >> PAGESHIFT, DOWN_TO_PAGE(new_brk), DOWN_TO_PAGE(new_brk) >> PAGESHIFT);

	TracePrintf(1024, "KERNEL_STACK_BASE: %d (%d), KERNEL_STACK_LIMIT: %d (%d), KERNEL_STACK_PAGES: %d, KERNEL_STACK_SIZE: %d, USER_STACK_LIMIT: %d (%d)\n", KERNEL_STACK_BASE, KERNEL_STACK_BASE >> PAGESHIFT, KERNEL_STACK_LIMIT, KERNEL_STACK_LIMIT >> PAGESHIFT, KERNEL_STACK_PAGES, KERNEL_STACK_SIZE, USER_STACK_LIMIT, USER_STACK_LIMIT >> PAGESHIFT);

	TracePrintf(2048, "KernelPageTable: %d %d %d\n", KernelPageTable, &KernelPageTable, *&KernelPageTable);
	TracePrintf(2048, "UserPageTable: %d %d %d\n", UserPageTable, &UserPageTable, *&UserPageTable);
	//assign kernel to page Table
	int limit;
    
	//assign kernel text
    limit = (UP_TO_PAGE(etextAddr) >> PAGESHIFT) - PAGE_TABLE_LEN;
	for(index = (VMEM_1_BASE >> PAGESHIFT) - PAGE_TABLE_LEN; index < limit; index++)
	{
		struct pte PTE;
		PTE.valid = 1;
		PTE.pfn = index + PAGE_TABLE_LEN;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_READ | PROT_EXEC;
		KernelPageTable[index] = PTE;
		struct PhysicalPageNode *node = physicalPages[index + PAGE_TABLE_LEN];
		free(node);
        physicalPages[index + PAGE_TABLE_LEN] = NULL;
		numPhysicalPagesLeft --;
		TracePrintf(2048, "Allocate page for text: vpn(%d), pfn(%d)\n", index, PTE.pfn);
	}

	//assign kernel data and bss
	limit = (UP_TO_PAGE(new_brk) >> PAGESHIFT) - PAGE_TABLE_LEN;
	for(index = (UP_TO_PAGE(etextAddr) >> PAGESHIFT) - PAGE_TABLE_LEN; index <= limit; index++)
	{
		struct pte PTE;
		PTE.valid = 1; 
		PTE.pfn = index + PAGE_TABLE_LEN;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_READ | PROT_WRITE;
		KernelPageTable[index] = PTE;
		struct PhysicalPageNode *node = physicalPages[index + PAGE_TABLE_LEN];
		free(node);
        physicalPages[index + PAGE_TABLE_LEN] = NULL;
		numPhysicalPagesLeft --;
		TracePrintf(2048, "Allocate page for data: vpn(%d), pfn(%d)\n", index, PTE.pfn);
	}

	printKernelPageTable(2048);

	//assign kernel stack
	limit = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;
	for(index = UP_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT; index < limit; index++)
	{
		struct pte PTE;
		PTE.valid = 1;
		PTE.pfn = index;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_READ | PROT_WRITE;
		UserPageTable[index] = PTE;
		struct PhysicalPageNode *node = physicalPages[index];
		free(node);
        physicalPages[index] = NULL;
		numPhysicalPagesLeft --;
		TracePrintf(2048, "Allocate page for stack: vpn(%d), pfn(%d)\n", index, PTE.pfn);
	}

	printUserPageTable(2048);
	
	//use a linked list to store the available physica pages
	//physicalPageNodeHead = 0;
	//physicalPageNodeCurrent = 0;
	 
	TracePrintf(1024, "Number of physical pages available after allocate to Kernel: %d\n", numPhysicalPagesLeft);
	numPhysicalPagesLeft = 0;
	for(index = 0; index < numOfPagesAvailable; index++)
	{
		if(physicalPages[index] != NULL)
		{
			if(physicalPageNodeHead == NULL)
			{
				  physicalPageNodeHead = physicalPages[index];
			}
			else
			{
				 physicalPages[index] -> next = physicalPageNodeHead;
				 physicalPageNodeHead = physicalPages[index];
			}
			numPhysicalPagesLeft++;
		}
	}

	TracePrintf(1796, "Number of free physical pages available after building linked list: %d\n", numPhysicalPagesLeft);
	printPhysicalPageLinkedList();

	//Write the page table address to the register and enable virtual memory
	RCS421RegVal kernelPageTableAddress = (RCS421RegVal)KernelPageTable;
	WriteRegister(REG_PTR1, kernelPageTableAddress);
	RCS421RegVal userPageTableAddress = (RCS421RegVal)UserPageTable;
	WriteRegister(REG_PTR0, userPageTableAddress);
	WriteRegister(REG_VM_ENABLE, 1);

	//Running the idle process
	TracePrintf(512, "ExceptionStackFrame: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", frame->vector, frame->code, frame->addr, frame->psr, frame->pc, frame->sp, frame->regs);

	LoadProgram("idle", cmd_args, frame);
}

extern int Fork(void)
{
	TracePrintf(512, "Fork\n");
	return 0;
}

extern int Exec(char *filename, char **argvec)
{
	TracePrintf(512, "Exec: filename(%s), argvec(%s)\n", filename, argvec);
	return 0;
}

extern int Wait(int *status_ptr)
{
	TracePrintf(512, "Wait: status_ptr(%d)\n", *status_ptr);
	return 0;
}

extern int GetPid(void)
{
	TracePrintf(512, "GetPid\n");
	return 0;
}

extern int Brk(void *addr)
{
	TracePrintf(512, "Brk: addr(%d)\n", addr);
	return 0;
}

extern int Delay(int clock_ticks)
{
	TracePrintf(512, "Delay: clock_ticks(%d)\n", clock_ticks);
	return 0;
}

extern int TtyRead(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyRead: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}

extern int TtyWrite(int tty_id, void *buf, int len)
{
	TracePrintf(512, "TtyWrite: tty_id(%d), buf(%s), len(%d)\n", tty_id, buf, len);
	return 0;
}

/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame)
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char **cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int data_bss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    if ((fd = open(name, O_RDONLY)) < 0) {
	TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
	return (-1);
    }

    status = LoadInfo(fd, &li);
    TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
    switch (status) {
	case LI_SUCCESS:
	    break;
	case LI_FORMAT_ERROR:
	    TracePrintf(0,
		"LoadProgram: '%s' not in Yalnix format\n", name);
	    close(fd);
	    return (-1);
	case LI_OTHER_ERROR:
	    TracePrintf(0, "LoadProgram: '%s' other error\n", name);
	    close(fd);
	    return (-1);
	default:
	    TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
	    close(fd);
	    return (-1);
    }
    TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
	li.text_size, li.data_size, li.bss_size);
    TracePrintf(0, "entry 0x%lx\n", li.entry);

    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
	size += strlen(args[i]) + 1;
    }
    argcount = i;
    TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);

    /*
     *  Now save the arguments in a separate buffer in Region 1, since
     *  we are about to delete all of Region 0.
     */
    cp = argbuf = (char *)malloc(size);
    for (i = 0; args[i] != NULL; i++) {
	strcpy(cp, args[i]);
	cp += strlen(cp) + 1;
    }
  
    /*
     *  The arguments will get copied starting at "cp" as set below,
     *  and the argv pointers to the arguments (and the argc value)
     *  will get built starting at "cpp" as set below.  The value for
     *  "cpp" is computed by subtracting off space for the number of
     *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
     *  terminate the auxiliary vector, a NULL pointer terminating
     *  the argv pointers, and a NULL pointer terminating the envp
     *  pointers) times the size of each (sizeof(void *)).  The
     *  value must also be aligned down to a multiple of 8 boundary.
     */
    cp = ((char *)USER_STACK_LIMIT) - size;
    cpp = (char **)((unsigned long)cp & (-1 << 4));	/* align cpp */
    cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

    text_npg = li.text_size >> PAGESHIFT;
    data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n",
	text_npg, data_bss_npg, stack_npg);

    /*
     *  Make sure we will leave at least one page between heap and stack
     */
    if (MEM_INVALID_PAGES + text_npg + data_bss_npg + stack_npg
	+ 1 + KERNEL_STACK_PAGES >= PAGE_TABLE_LEN) {
	TracePrintf(0, "LoadProgram: program '%s' size too large for VM\n",
	    name);
	free(argbuf);
	close(fd);
	return (-1);
    }

    /*
     *  And make sure there will be enough physical memory to
     *  load the new program.
     */
    /*
	>>>> The new program will require text_npg pages of text,
    >>>> data_bss_npg pages of data/bss, and stack_npg pages of
    >>>> stack.  In checking that there is enough free physical
    >>>> memory for this, be sure to allow for the physical memory
    >>>> pages already allocated to this process that will be
    >>>> freed below before we allocate the needed pages for
    >>>> the new program being loaded.
	*/
	int totalPageNeeded = text_npg + data_bss_npg + stack_npg;
	TracePrintf(1536, "Total Page Needed: %d\n", totalPageNeeded);

    if (numPhysicalPagesLeft < totalPageNeeded) {
	TracePrintf(0,
	    "LoadProgram: program '%s' size too large for physical memory\n",
	    name);
	free(argbuf);
	close(fd);
	return (-1);
    }

    //>>>> Initialize sp for the current process to (char *)cpp.
    //>>>> The value of cpp was initialized above.
	frame -> sp = (char *)cpp;
	TracePrintf(1536, "Set frame -> sp\n");

    /*
     *  Free all the old physical memory belonging to this process,
     *  but be sure to leave the kernel stack for this process (which
     *  is also in Region 0) alone.
     */
    /*
	>>>> Loop over all PTEs for the current process's Region 0,
    >>>> except for those corresponding to the kernel stack (between
    >>>> address KERNEL_STACK_BASE and KERNEL_STACK_LIMIT).  For
    >>>> any of these PTEs that are valid, free the physical memory
    >>>> memory page indicated by that PTE's pfn field.  Set all
    >>>> of these PTEs to be no longer valid.
	*/
	int index = 0;
	for (index = 0; index < KERNEL_STACK_BASE >> PAGESHIFT; index ++)
	{
		struct pte *PTE = &UserPageTable[index];
		if(PTE -> valid == 1)
		{
			int pfn = PTE -> pfn;
			freePhysicalPage(pfn);
		}
		PTE -> valid = 0;
	}
	TracePrintf(1536, "Clear Region 0\n");

    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    //>>>> Leave the first MEM_INVALID_PAGES number of PTEs in the
    //>>>> Region 0 page table unused (and thus invalid)
	index = 0;
	for (index = 0; index < MEM_INVALID_PAGES; index ++)
	{
		struct pte *PTE = &UserPageTable[index];
		PTE -> valid = 0;
		PTE -> pfn = 0;
	}
	TracePrintf(1536, "Set first MEM_INVALID_PAGES invalid.\n");

    /* First, the text pages */
	/*
    >>>> For the next text_npg number of PTEs in the Region 0
    >>>> page table, initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_EXEC
    >>>>     pfn   = a new page of physical memory
	*/
	for( index = MEM_INVALID_PAGES; index < MEM_INVALID_PAGES + text_npg; index ++)
	{
		struct pte *PTE = &UserPageTable[index];
		PTE -> valid = 1;
		PTE -> kprot = PROT_READ | PROT_WRITE;
		PTE -> uprot = PROT_READ | PROT_EXEC;
		PTE -> pfn = allocatePhysicalPage();
	}
	TracePrintf(1536, "Initialize text pages\n");

    /* Then the data and bss pages */
	/*
    >>>> For the next data_bss_npg number of PTEs in the Region 0
    >>>> page table, initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_WRITE
    >>>>     pfn   = a new page of physical memory
	*/
	int boundry = index;
	for(index = boundry; index < boundry + data_bss_npg; index++)
	{
		struct pte *PTE = &UserPageTable[index];
		PTE -> valid = 1;
		PTE -> kprot = PROT_READ | PROT_WRITE;
		PTE -> uprot = PROT_READ | PROT_WRITE;
		PTE -> pfn = allocatePhysicalPage();
	}
	TracePrintf(1536, "Initialize data and bss pages\n");

    /* And finally the user stack pages */
	/*
    >>>> For stack_npg number of PTEs in the Region 0 page table
    >>>> corresponding to the user stack (the last page of the
    >>>> user stack *ends* at virtual address USER_STACK_LMIT),
    >>>> initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_WRITE
    >>>>     pfn   = a new page of physical memory
	*/
	TracePrintf(1792, "USER_STACK_LIMIT: %d, USER_STACK_LIMIT >> PAGESHIFT: %d, stack_npg: %d", USER_STACK_LIMIT, USER_STACK_LIMIT >> PAGESHIFT, stack_npg);
	for(index = (USER_STACK_LIMIT >> PAGESHIFT); index >= (USER_STACK_LIMIT >> PAGESHIFT) - stack_npg; index --)
	{
		TracePrintf(1792, "Allocate %d for user stack, total %d pages\n", index, stack_npg);
		struct pte *PTE = &UserPageTable[index];
		PTE -> valid = 1;
		PTE -> kprot = PROT_READ | PROT_WRITE;
		PTE -> uprot = PROT_READ | PROT_WRITE;
		PTE -> pfn = allocatePhysicalPage();
	}
	TracePrintf(1536, "Initialize stack pages\n");
	printUserPageTable(1920);

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size)
	!= li.text_size+li.data_size) {
	TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
	free(argbuf);
	close(fd);
	/*
	>>>> Since we are returning -2 here, this should mean to
	>>>> the rest of the kernel that the current process should
	>>>> be terminated with an exit status of ERROR reported
	>>>> to its parent process.
	*/
	return (-2);
    }
	TracePrintf(1536, "Read the text and data from the file into memory\n");

    close(fd);			/* we've read it all now */

    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */
	/*
    >>>> For text_npg number of PTEs corresponding to the user text
    >>>> pages, set each PTE's kprot to PROT_READ | PROT_EXEC.
	*/
	for( index = MEM_INVALID_PAGES; index < MEM_INVALID_PAGES + text_npg; index ++)
	{
		  struct pte *PTE = &UserPageTable[index];
		  PTE -> kprot = PROT_READ | PROT_EXEC;
	}
	TracePrintf(1536, "Make the Kernel PROT_READ | PROT_EXEC\n");

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
	'\0', li.bss_size);

    /*
     *  Set the entry point in the exception frame.
     */
    //>>>> Initialize pc for the current process to (void *)li.entry
	frame -> pc = (void *)li.entry;
	TracePrintf(1536, "Set frame -> pc\n");

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
	*cpp++ = cp;
	strcpy(cp, cp2);
	cp += strlen(cp) + 1;
	cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;	/* the last argv is a NULL pointer */
    *cpp++ = NULL;	/* a NULL pointer for an empty envp */
    *cpp++ = 0;		/* and terminate the auxiliary vector */

    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     */
	/*
    >>>> Initialize regs[0] through regs[NUM_REGS-1] for the
    >>>> current process to 0.
    >>>> Initialize psr for the current process to 0.
	*/
	frame -> psr = 0;

	for(index = 0; index < NUM_REGS; index++)
	{
		frame -> regs[index] = 0;
	}
	TracePrintf(1536, "Initialize regs[]\n");

    return (0);
}



