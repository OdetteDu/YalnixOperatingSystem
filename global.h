//interrupt vector table
void (*interruptTable[TRAP_VECTOR_SIZE])(ExceptionStackFrame *);

//Kernel brk
void *new_brk;

//Page Tables
struct pte *KernelPageTable;
struct pte *UserPageTable;

//Current process
int currentPID;
SavedContext currentSavedContext;

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
	  struct pte *pageTable;
	  SavedContext ctxp;
	  struct PCBNode *next;
};

struct PCBNode *readyQuqueHead;
struct PCBNode *readyQueueTail;

