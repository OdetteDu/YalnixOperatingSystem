#include <stdlib.h>
#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include "util.h"


/* Global var declaration */
extern struct PhysicalPageNode* physicalPageNodeHead;
extern struct pte *UserPageTable;
extern struct pte *KernelPageTable;

/*Queue util */
extern void addToQEnd(struct queue* topush, struct queue* qTail)
{
	qTail->next = topush;
	qTail = topush;
}

extern struct PCBNode* popQHead(struct queue* qHead)
{
	//printf("POPING QHEAD:  %d\n", qHead);
	struct PCBNode* temp = qHead->proc;
	qHead = qHead->next;
	//printf("POPING QHEAD: temp %d\n", temp->PID);
	return temp;
}

/* Physical page node util */
//Allocate and free physical pages
extern int allocatePhysicalPage()
{
	struct PhysicalPageNode *allocatedPhysicalPageNode = physicalPageNodeHead;
	physicalPageNodeHead = allocatedPhysicalPageNode -> next;
	int number = allocatedPhysicalPageNode -> pageNumber;
	free(allocatedPhysicalPageNode);
	numPhysicalPagesLeft--;
	TracePrintf(1536, "Allocated physical page number %d\n", number);
	return number;
}

extern void freePhysicalPage(int pfn)
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

/* Debugging util */
extern void printPhysicalPageLinkedList()
{
	TracePrintf(3072, "*********Print Free Physical Pages: *************\n");
	struct PhysicalPageNode *current = physicalPageNodeHead;
	while(current != NULL)
	{
		TracePrintf(3072, "%d\n", current -> pageNumber);
		current = current -> next;
	}
}

extern void printKernelPageTable(int level)
{
	TracePrintf(level, "****Print Kernel Page Table*****\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, KernelPageTable[index].valid, KernelPageTable[index].pfn);
	}
}

extern void printUserPageTable(int level)
{
	TracePrintf(level, "*****Print User Page Table*******\n");
	int index;
	for(index = 0; index < PAGE_TABLE_LEN; index++)
	{
		TracePrintf(level, "%d: valid(%d), pfn(%d), kprot(%d), uprot(%d)\n", index, UserPageTable[index].valid, UserPageTable[index].pfn, UserPageTable[index].kprot, UserPageTable[index].uprot);
	}
}


