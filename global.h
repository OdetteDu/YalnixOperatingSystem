#ifndef _global_h
#define _global_h
//interrupt vector table
extern void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

//Kernel brk
extern void *new_brk;

//Page Tables
extern struct pte *KernelPageTable;
extern struct pte *UserPageTable;

//Current process
extern int currentPID;
extern SavedContext currentSavedContext;

//Available Physical Pages
struct PhysicalPageNode
{
	  int pageNumber;
	  struct PhysicalPageNode *next;
};

extern int numPhysicalPagesLeft;
extern struct PhysicalPageNode *physicalPageNodeHead;

//PCB
struct PCBNode
{
	  int PID;
	  struct pte *pageTable;
	  SavedContext ctxp;
	  struct PCBNode *next;
};

extern struct PCBNode *readyQuqueHead;
extern struct PCBNode *readyQueueTail;
#endif /* end _global_h */

