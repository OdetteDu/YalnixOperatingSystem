#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <comp421/loadinfo.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include "kernel_call.h"
#include "util.h"

/* Initlize variables */
//physical pages
int numPhysicalPagesLeft;
struct PhysicalPageNode *physicalPageNodeHead;

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

//Current process
struct PCBNode *idle;
struct PCBNode *current;

struct PCBNode *readyQuqueHead;
struct PCBNode *readyQueueTail;

/* Function declaration */
extern SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2);

extern int nextPID()
{ 
	return currentPID++;
}

/*
extern SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2)
{
 
  ((struct PCBNode *)p1) -> PID = currentPID;
  ((struct PCBNode *)p1) -> pageTable = UserPageTable;
  ((struct PCBNode *)p1) -> ctxp = currentSavedContext;
  currentPID = ((struct PCBNode *)p2) -> PID;
  UserPageTable = ((struct PCBNode *)p2) -> pageTable;
  currentSavedContext = ((struct PCBNode *)p2) -> ctxp;
  RCS421RegVal userPageTableAddress = (RCS421RegVal)UserPageTable;
  WriteRegister(REG_PTR0, userPageTableAddress);
  return &currentSavedContext; 
}
*/

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
	
  KernelPageTable = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
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

  UserPageTable = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
  for( index = 0; index < PAGE_TABLE_LEN; index++ )
	{
      struct pte PTE;
      PTE.valid = 0;
      PTE.pfn = 0;
      PTE.uprot = PROT_NONE;
      PTE.kprot = PROT_NONE;
      UserPageTable[index] = PTE;
    }

  InitPageTable = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
  for( index = 0; index < PAGE_TABLE_LEN; index++ )
    {
      struct pte PTE;
      PTE.valid = 0;
      PTE.pfn = 0;
      PTE.uprot = PROT_NONE;
      PTE.kprot = PROT_NONE;
      InitPageTable[index] = PTE;
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
  TracePrintf(2048, "InitPageTable: %d %d %d\n", InitPageTable, &InitPageTable, *&InitPageTable);
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
  RCS421RegVal UserPageTableAddress = (RCS421RegVal)UserPageTable;
  WriteRegister(REG_PTR0, UserPageTableAddress);
  WriteRegister(REG_VM_ENABLE, 1);
  vm_enabled = 1;

  //Running the idle process
  TracePrintf(512, "ExceptionStackFrame: vector(%d), code(%d), addr(%d), psr(%d), pc(%d), sp(%d), regs(%s)\n", frame->vector, frame->code, frame->addr, frame->psr, frame->pc, frame->sp, frame->regs);
  
  LoadProgram("idle", cmd_args, frame);
  /* build idle and init */

  //strcut PCBNode *p1 = buildPCB(ACTIVE, UserPageTable
  return;
}





