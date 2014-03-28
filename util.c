#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include "util.h"


/* Global var declaration */

struct PhysicalPageNode* physicalPageNodeHead;
struct pte *UserPageTable;
struct pte *KernelPageTable;

extern void printPhysicalPageLinkedList()
{
	TracePrintf(3072, "Free Physical Pages: \n");
	struct PhysicalPageNode *current = physicalPageNodeHead;
	while(current != NULL)
	{
		TracePrintf(3072, "%d\n", current -> pageNumber);
		current = current -> next;
	}
}

extern void printKernelPageTable(int level)
{
	TracePrintf(level, "Print Kernel Page Table\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		  TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, KernelPageTable[index].valid, KernelPageTable[index].pfn);
	}
}

extern void printUserPageTable(int level)
{
	TracePrintf(level, "Print User Page Table\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		  TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, UserPageTable[index].valid, UserPageTable[index].pfn);
	}
}
