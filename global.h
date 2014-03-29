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
extern struct PhysicalPageNode *physicalPageNodeTail;

/* Flags for process status */
#define READY 1
#define BLOCKED 2
#define ACTIVE 3
#define TERMINATED 4


//PCB
struct PCBNode
{
  int PID;
  struct pte *pageTable;
  unsigned int stack_brk;
  unsigned int heap_brk;
  SavedContext ctxp;
  struct PCBNode **children;//list of children
  struct PCBNode *parent;
  int status;
  int idle;
  int clockCount; //decides for switching.
};

extern struct PCBNode *readyQuqueHead;
extern struct PCBNode *readyQueueTail;
extern int nextPID();
/* Physical page node functions */
extern int allocatePhysicalPage();
extern void freePhysicalPage(int pfn);

/* PCBNode functions */
//extern PCBNode* buildPCB(struct pte *pageTable, int status);
extern void addFirstToReadyQueue(int pid, struct pte *pageTable, SavedContext ctxp);
extern void addLastToReadyQueue(int pid, struct pte *pageTable, SavedContext ctxp);
extern struct PCBNode *removeFirstFromReadyQueue();

#endif /* end _global_h */

