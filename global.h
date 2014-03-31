#ifndef _global_h
#define _global_h
//interrupt vector table
extern void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);



//PID Generator
extern unsigned int PIDGenerator;
extern int nextPID();

//Kernel brk
extern void *new_brk;

//Page Tables
extern struct pte *KernelPageTable;
extern struct pte *UserPageTable;

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
  int isActive;
  int blockedReason;
  int numTicksRemainForDelay; 

  struct exitStatusQ* exitStatusQ;

  struct PCBNode *parent;
  struct PCBNode *prevSibling;
  struct PCBNode *nextSibling;
  struct PCBNode *child;
struct queue* children;
};

extern struct PCBNode* active_process;
extern struct PCBNode* idle;
extern struct PCBNode* init;

/* Process queue */
struct queue{
	struct PCBNode* proc;
	struct queue* next;
};

struct exitStatusQ{
	int exitStatus;
	int PID;
	struct exitStatusQ* next;
};

extern struct queue *waitingQHead, *waitingQTail;
extern struct queue *readyQHead, *readyQTail;
extern struct queue *delayQueueHead, *delayQueueTail;

extern void addToQEnd(struct queue* topush, struct queue* qTail);
extern struct PCBNode* popQHead(struct queue* qHead);

//extern queue *readyQueueHead, readyQueueTail

/* Physical page node functions */
extern int allocatePhysicalPage();
extern void freePhysicalPage(int pfn);


/* PCBNode functions */


/* Switch util */

extern SavedContext *generalSwitchFunc(SavedContext *ctxp, void *p1, void *p2);
extern SavedContext *forkSwitchFunc(SavedContext *ctxp, void *p1, void *p2);
extern SavedContext *delaySwitchFunc(SavedContext *ctxp, void *p1, void *p2);
extern SavedContext *exitSwitchFunc(SavedContext *ctxp, void *p1, void *p2);
#endif /* end _global_h */



//extern queue *readyQueueHead, readyQueueTail
