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
extern unsigned int currentPID;
extern SavedContext currentSavedContext;

//Available Physical Pages
struct PhysicalPageNode
{
  int pageNumber;
  struct PhysicalPageNode *next;
};

extern int numPhysicalPagesLeft;
extern struct PhysicalPageNode *physicalPageNodeHead;

/* Flags for process status */
#define READY 1
#define BLOCKED 2
#define ACTIVE 3
#define TERMINATED 4

#define Delay 1
#define Wait 2
#define TTYRead 3
#define TTYWrite 4

//PCB
struct PCBNode
{
  int PID;
  SavedContext ctxp;

  struct pte *pageTable;
  unsigned int stack_brk;
  unsigned int heap_brk;

  int status;
  int blockedReason;
  int numTicksRemainForDelay; 

  struct PCBNode *parent;
  struct PCBNode *prevSibling;
  struct PCBNode *nextSibling;
  struct PCBNode *children;
};

extern int nextPID();

extern struct PCBNode *readyQuqueHead;
extern struct PCBNode *readyQueueTail;

/* Physical page node functions */
extern int allocatePhysicalPage();
extern void freePhysicalPage(int pfn);

/* PCBNode functions */
//extern PCBNode* buildPCB(struct pte *pageTable, int status);
extern void addFirstToReadyQueue(int pid, struct pte *pageTable, SavedContext ctxp);
extern void addLastToReadyQueue(int pid, struct pte *pageTable, SavedContext ctxp);
extern struct PCBNode *removeFirstFromReadyQueue();

#endif /* end _global_h */

