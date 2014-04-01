#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include "kernel_call.h"
#include "util.h"

/* Initlize variables */
unsigned int count = 0;

//physical pages
int numPhysicalPagesLeft;
struct PhysicalPageNode *physicalPageNodeHead;

//PID Generator
unsigned int PIDGenerator;

//VM flag
unsigned int vm_enabled;

//interrupt vector table
void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

//Kernel brk
void *new_brk;

//Page Tables
struct pte *KernelPageTable;
struct pte *UserPageTable;
struct pte *InitPageTable;
struct pte *IdlePageTable;
struct pte myKernelPageTable[PAGE_TABLE_LEN];
struct pte myUserPageTable[PAGE_TABLE_LEN];
struct pte myInitPageTable[PAGE_TABLE_LEN];
struct pte myIdlePageTable[PAGE_TABLE_LEN];

//physical pages
struct PhysicalPageNode *physicalPageNodeHead;
struct PhysicalPageNode *physicalPageNodeTail;

//Current process
struct PCBNode *idle;
struct PCBNode *active_process;
struct PCBNode *init;

//Process Queue
struct queue *waitingQHead, *waitingQTail;
struct queue *readyQHead, *readyQTail;
struct queue *delayQueueHead, *delayQueueTail;


/* Function declaration */
extern int LoadProgram(char *name, char **args, ExceptionStackFrame *frame);

extern int nextPID()
{ 
	return PIDGenerator++;
}

extern SavedContext *exitSwitchFunc(SavedContext *ctxp, void* p1, void* p2)
{
	TracePrintf(0, "[Exit Switch] Entrance \n");
	struct pte* table2 = ((struct PCBNode*)p2)->pageTable;
	if(p2==0){p2 = idle;}
	memcpy(UserPageTable, table2, PAGE_TABLE_LEN);
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
	freePhysicalPage(((struct PCBNode*)p1)->pageTable);
	free(((struct PCBNode*)p1));
	((struct PCBNode*)p2)->status = ACTIVE;
	active_process = ((struct PCBNode*)p2);
	return  &(((struct PCBNode*)p2)->ctxp);
}

/* Only Difference with generalSwitch is we push delay queue*/
extern SavedContext *delaySwitchFunc(SavedContext *ctxp, void* p1, void* p2)
{
	struct pte *table1 = ((struct PCBNode*)p1)->pageTable;
	struct pte *table2 = ((struct PCBNode*)p2)->pageTable;
	TracePrintf(256, "[Delay Switch] Entrance\n");

	//check if p2 is null
	if(!p2)
	{
		return &(((struct PCBNode*)p1)->ctxp);
	}

	UserPageTable = table2;
	WriteRegister(REG_PTR0, (RCS421RegVal)table2);
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
	((struct PCBNode*)p2)->status = ACTIVE;
	((struct PCBNode*)p1)->status = READY;
	active_process = p2;
	memcpy(&(((struct PCBNode*)p1)->ctxp), ctxp, sizeof(SavedContext));

	//TODO: put p1 in ready queue
	struct queue* pcb1 = malloc(sizeof(struct queue));//should check for if p1 is idle here ..
	pcb1->proc = ((struct PCBNode*)p1);
	pcb1->next = NULL;

	if(delayQueueHead == NULL)
	{
		delayQueueHead = pcb1;
		delayQueueTail = pcb1;
	}
	else
	{
		delayQueueTail->next = pcb1;
		delayQueueTail = pcb1;
	}

	printf("[DelaySwitched] from pid: %d to pid: %d\n", ((struct PCBNode*)p1)->PID, ((struct PCBNode*)p2)->PID);
	return &(((struct PCBNode*)p2)->ctxp);
}

extern SavedContext *generalSwitchFunc(SavedContext *ctxp, void* p1, void* p2)
{
	struct pte *table1 = ((struct PCBNode*)p1)->pageTable;
	struct pte *table2 = ((struct PCBNode*)p2)->pageTable;
	TracePrintf(256, "[General Switch] Entrance\n");

	//check if p2 is null
	if(!p2)
	{
		return &(((struct PCBNode*)p1)->ctxp);
	}

	UserPageTable = table2;
	WriteRegister(REG_PTR0, (RCS421RegVal)table2);
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
	((struct PCBNode*)p2)->status = ACTIVE;
	((struct PCBNode*)p1)->status = READY;
	active_process = p2;
	memcpy(&(((struct PCBNode*)p1)->ctxp), ctxp, sizeof(SavedContext));
	//TODO: put p1 in ready queue
	struct queue* pcb1 = malloc(sizeof(struct queue));//should check for if p1 is idle here ..
	pcb1->proc = ((struct PCBNode*)p1);
	pcb1->next = NULL;
	if(readyQHead == NULL)
	{
		readyQHead = pcb1;
		readyQTail = pcb1;
	}
	else
	{
		readyQTail->next = pcb1;
		readyQTail = pcb1;
	}
	printf("[General Switched] from pid: %d to pid: %d\n", ((struct PCBNode*)p1)->PID, ((struct PCBNode*)p2)->PID);
	return &(((struct PCBNode*)p2)->ctxp);
}
/**
 * @param: p1 should be the parent process and p2 should be the child
 */
extern SavedContext *forkSwitchFunc(SavedContext *ctxp, void *p1, void *p2)
{
	struct pte *table1 = ((struct PCBNode*)p1)->pageTable;
	struct pte *table2 = ((struct PCBNode*)p2)->pageTable;

	unsigned int i;
	uint64_t *buffer[PAGE_TABLE_LEN];

	TracePrintf(256, "[Debug] Enter forkSwitch:\n");
	/* Copy all valid pages from the UserPageTable
	 *  We must call fork from the active process.
	 **/

	printf("check if table2 is allocated table2[0] = %d\n", table2[0]);
	for(i=0; i<PAGE_TABLE_LEN;i++)
	{
		table2[i].valid = table1[i].valid;
		if(table1[i].valid)
		{//allow for full access in new table first
			table2[i].kprot = PROT_READ | PROT_WRITE;
			table2[i].uprot = PROT_NONE;
			buffer[i] = malloc(PAGESIZE);
			TracePrintf(256, "[Fork Switch Copy]: allocated memory for the %d page %p\n", i, buffer[i]);
			memcpy(buffer[i], (uint64_t *)(i<<PAGESHIFT), PAGESIZE);
		}
	}
	printUserPageTable(1024);

	//swap region0 now;
	UserPageTable = table2;
	printUserPageTable(100);
	long vpn = ((long)table2 & PAGEMASK) >> PAGESHIFT; 
	int pfn = KernelPageTable[vpn-512].pfn;
	long offset = (long)table2 & PAGEOFFSET;
	long addrUP = pfn << PAGESHIFT;
	long addr = addrUP + offset;
	TracePrintf(100, "table2: %d, vpn: %d, pfn: %d, offset: %d, addrUP: %d, addr: %d\n", table2, vpn, pfn, offset, addrUP, addr);
	WriteRegister(REG_PTR0, (RCS421RegVal)addr);
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
	printUserPageTable(1024);

	TracePrintf(256, "[Fork Switch Copy] buffer completed \n");

	/* Copy back from buffer */
	for(i=0;i<PAGE_TABLE_LEN;i++)
	{
		if(UserPageTable[i].valid)
		{
			//allocate a new page for this pte
			int newpfn = allocatePhysicalPage();
			UserPageTable[i].pfn = newpfn;
			memcpy((uint64_t *)(i<<PAGESHIFT), buffer[i], PAGESIZE);
			free(buffer[i]);//free is internal to malloc?? does not call setkernelpagebrk;
			//leave it as it is
			TracePrintf(256, "[Fork Swtich Copy]: freed memory for the %d page \n", i);
			UserPageTable[i].kprot = table1[i].kprot;// PROT_READ | PROT_WRITE;
			UserPageTable[i].uprot = table1[i].uprot;//PROT_NONE;
		}
	}

	memcpy(&(((struct PCBNode*)p2)->ctxp), ctxp, sizeof(SavedContext));
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
	active_process = p2;

	/* Maintain the parent/children part??*/
	struct queue* child = malloc(sizeof(struct queue));
	child->proc = ((struct PCBNode*)p2);
	child->next = NULL;
	if(((struct PCBNode*)p1)->children == 0)
	{
		((struct PCBNode*)p1)->children = child;
	}
	else
	{
		(((struct PCBNode*)p1)->children)->next = child; //put in child list
	}

	((struct PCBNode*)p2)->parent = ((struct PCBNode*)p1);
	//TODO: write a process queue and put p1 in the ready queue
	//TODO: put p1 in ready queue
	struct queue* pcb1 = malloc(sizeof(struct queue));//should check for if p1 is idle here ..
	pcb1->proc = ((struct PCBNode*)p1);
	pcb1->next = NULL;

	if(readyQHead == 0)
	{
		readyQHead = pcb1;
		readyQTail = pcb1;
	}
	else
	{
		readyQTail->next = pcb1;
		readyQTail = pcb1;
	}

	printf("ForkSwitched from pid: %d to pid: %d\n", ((struct PCBNode*)p1)->PID, ((struct PCBNode*)p2)->PID);
	return &(((struct PCBNode*)p2)->ctxp);
}


extern int SetKernelBrk(void *addr)
{
	TracePrintf(512, "Set Kernel Brk Called: addr >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)addr >> PAGESHIFT, UP_TO_PAGE(addr), UP_TO_PAGE(addr) >> PAGESHIFT, DOWN_TO_PAGE(addr), DOWN_TO_PAGE(addr) >> PAGESHIFT);

	if(vm_enabled==1){

		//if(addr>VMEM_1_LIMIT) return -1;
		if(UP_TO_PAGE(addr) > VMEM_1_LIMIT)
		{
			TracePrintf(512, "Error: Kernel heap out of memory");
			return -1;
		}

		unsigned int kernelTablePte;
		//Only allocate for entire pages??
		unsigned int gap = (UP_TO_PAGE(addr)-UP_TO_PAGE(new_brk));
		if(gap>0)
		{
			TracePrintf(512, "Set Kernel Brk Called: moving up to %d", addr);
			for(kernelTablePte = (UP_TO_PAGE(new_brk)); kernelTablePte< (UP_TO_PAGE(addr)); kernelTablePte += PAGESIZE)
			{
				unsigned int i = ((kernelTablePte)>>PAGESHIFT)%PAGE_TABLE_LEN;
				KernelPageTable[i].valid = 1;
				KernelPageTable[i].uprot = PROT_NONE;
				KernelPageTable[i].kprot = PROT_READ | PROT_WRITE;
				/* Need to change the pfn here */
				KernelPageTable[i].pfn = allocatePhysicalPage();
				TracePrintf(1024, "[Debug] moved up kernel_brk to pagenumber %d, with pfn %d\n", i, KernelPageTable[i].pfn);
			}
		}
		else if(gap<0)
		{
			TracePrintf(512, "Set Kernel Brk Called: moving down ");
			for(kernelTablePte = (UP_TO_PAGE(addr)); kernelTablePte <(UP_TO_PAGE(new_brk)); kernelTablePte+=PAGESIZE)
			{
				unsigned int i =((kernelTablePte)>>PAGESHIFT)%PAGE_TABLE_LEN;
				KernelPageTable[i].valid = 0;
				freePhysicalPage (KernelPageTable[i].pfn);
				WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
			}
		}
		new_brk = addr;//finally, set the address to the new break
	}
	else
	{// this is when virtual memory is not declared.
		TracePrintf(1020, "Set Kernel Brk Called: addr >> PAGESHIFT: %d, UP_TO_PAGE: %d (Page:%d), DOWN_TO_PAGE:%d(Page:%d)\n", (long)addr >> PAGESHIFT, UP_TO_PAGE(addr), UP_TO_PAGE(addr) >> PAGESHIFT, DOWN_TO_PAGE(addr), DOWN_TO_PAGE(addr) >> PAGESHIFT);
		if(new_brk == NULL)
		{
			new_brk = addr;
			TracePrintf(1020, "Set new_brk from NULL to %d\n", new_brk);
		}
		else
		{
			if(addr > new_brk)
			{
				TracePrintf(1020, "Set new_brk from %d to %d\n", new_brk, addr);
				new_brk = addr;
			}
		}

	}
	TracePrintf(0, "finish set kernel brk!\n");
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
	TracePrintf(3072, "Total number of physical pages: %d, Available pages: %d\n", numOfPagesAvailable, numPhysicalPagesLeft);

	struct PhysicalPageNode *physicalPages[numOfPagesAvailable];
	for ( index=0; index<numOfPagesAvailable; index++)
	{
		struct PhysicalPageNode *newNode;
		newNode = (struct PhysicalPageNode *) malloc(sizeof(struct PhysicalPageNode));
		newNode -> pageNumber = index;
		newNode -> next = NULL;
		physicalPages[index] = newNode;
	}

	//allocate kernelpagetable
	KernelPageTable =myKernelPageTable;// malloc(PAGE_TABLE_LEN * sizeof(struct pte));
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
	TracePrintf(2048, "KernelPageTable: Address: %d, PAGE_TABLE_LEN: %d, KernelPageTable Size: %d\n", KernelPageTable, PAGE_TABLE_LEN, sizeof(KernelPageTable));

	//allocate userpagetable
	UserPageTable = myUserPageTable;//malloc(PAGE_TABLE_LEN * sizeof(struct pte));
	for( index = 0; index < PAGE_TABLE_LEN; index++ )
	{
		struct pte PTE;
		PTE.valid = 0;
		PTE.pfn = 0;
		PTE.uprot = PROT_NONE;
		PTE.kprot = PROT_NONE;
		UserPageTable[index] = PTE;
	}
	TracePrintf(2048, "UserPageTable: Address: %d, PAGE_TABLE_LEN: %d, UserPageTable Size: %d\n", UserPageTable, PAGE_TABLE_LEN, sizeof(UserPageTable));

	//allocate initpage table
	InitPageTable = myInitPageTable;//malloc(PAGE_TABLE_LEN * sizeof(struct pte));
	IdlePageTable = myIdlePageTable;//malloc(PAGE_TABLE_LEN * sizeof(struct pte));

	TracePrintf(2048, "InitPageTable: Address: %d, PAGE_TABLE_LEN: %d, InitPageTable Size: %d\n", InitPageTable, PAGE_TABLE_LEN, sizeof(InitPageTable));
	TracePrintf(2000, "KERNEL_STACK_BASE: %d (%d), KERNEL_STACK_LIMIT: %d (%d), KERNEL_STACK_PAGES: %d, KERNEL_STACK_SIZE: %d, USER_STACK_LIMIT: %d (%d)\n", KERNEL_STACK_BASE, KERNEL_STACK_BASE >> PAGESHIFT, KERNEL_STACK_LIMIT, KERNEL_STACK_LIMIT >> PAGESHIFT, KERNEL_STACK_PAGES, KERNEL_STACK_SIZE, USER_STACK_LIMIT, USER_STACK_LIMIT >> PAGESHIFT);
	TracePrintf(2048, "KernelPageTable: %d %d %d\n", KernelPageTable, &KernelPageTable, *KernelPageTable);
	TracePrintf(2048, "UserPageTable: %d %d %d\n", UserPageTable, &UserPageTable, *UserPageTable);
	TracePrintf(2048, "InitPageTable: %d %d %d\n", InitPageTable, &InitPageTable, *InitPageTable);

	//assign kernel to page Table
	long etextAddr = (long)&_etext;
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

	printKernelPageTable(2044);

	//assign Kernel Stack for UserPageTable
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
		TracePrintf(2048, "Allocate page for stack in UserPageTable: vpn(%d), pfn(%d)\n", index, PTE.pfn);
	}

	printUserPageTable(2044);

	TracePrintf(3070, "Number of physical pages available after allocate to Kernel: %d\n", numPhysicalPagesLeft);
	numPhysicalPagesLeft = 0;
	for(index = 0; index < numOfPagesAvailable; index++)
	{
		if(physicalPages[index] != NULL)
		{
			if(physicalPageNodeHead == NULL)
			{
				physicalPageNodeHead = physicalPages[index];
				physicalPageNodeTail = physicalPageNodeHead;
			}
			else
			{
				physicalPageNodeTail->next = physicalPages[index];
				physicalPageNodeTail = physicalPages[index];
			}
			numPhysicalPagesLeft++;
			TracePrintf(1024, "Free physical page linked : index %d, pagenumber %d\n", index,
					physicalPageNodeTail->pageNumber);
		}
	}

	TracePrintf(1024, "Number of free physical pages available after building linked list: %d\n", numPhysicalPagesLeft);
	printPhysicalPageLinkedList();

	//Write the page table address to the register and enable virtual memory
	RCS421RegVal kernelPageTableAddress = (RCS421RegVal)KernelPageTable;
	WriteRegister(REG_PTR1, kernelPageTableAddress);
	RCS421RegVal UserPageTableAddress = (RCS421RegVal)UserPageTable;
	WriteRegister(REG_PTR0, UserPageTableAddress);
	WriteRegister(REG_VM_ENABLE, 1);
	vm_enabled = 1;

	//Running the idle process
	TracePrintf(512, "ExceptionStackFrame: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", frame->vector, frame->code, frame->addr, frame->psr, frame->pc, frame->sp, frame->regs);

	/* build idle and init */
	idle = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	idle -> PID = 0;
	idle -> status = READY;
	idle -> pageTable = myUserPageTable;
	idle -> isActive = 1;
	idle -> blockedReason = 0;
	idle -> numTicksRemainForDelay = 0;
	idle -> parent = NULL;
	idle -> child = NULL;
	idle -> prevSibling = NULL;
	idle -> nextSibling = NULL;
	active_process = idle;
	LoadProgram("idle", cmd_args, frame);//need to set the stack_brk and heap_brk in LoadProgram

	struct PCBNode* current;
	current = (struct PCBNode *)malloc(sizeof(struct PCBNode));
	current -> PID = 1;
	current -> pageTable =InitPageTable;
	current -> status = 1;
	current -> blockedReason = 0;
	current -> numTicksRemainForDelay = 0;
	current -> parent = NULL;
	current -> child = NULL;
	current -> prevSibling = NULL;
	current -> nextSibling = NULL;
	init = current;

	ContextSwitch(forkSwitchFunc, &(active_process->ctxp), active_process, init);
	TracePrintf(512, "[Debug] Context switched from idle to init");

	if(count == 0)
	{
		LoadProgram("init", cmd_args, frame);
		count =1;
	}

	return;
}





