#include <stdlib.h>
#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "trap_handler.h"
#include "global.h"
#include "util.h"


/* Global var declaration */
struct PhysicalPageNode* physicalPageNodeHead;
struct pte *UserPageTable;
struct pte *KernelPageTable;


/* Queue util */
extern void addToQEnd(struct queue* topush, struct queue* qTail){
	qTail->next = topush;
	topush = qTail;
}

extern struct PCBNode* popQHead(struct queue* qHead){
	struct queue* temp = qHead;
	qHead = temp->next;
	return temp->proc;
}

/* TTYQueue util */
extern void addToTTYQEnd(struct TTYQueue* topush, struct TTYQueue* qTail)
{
	qTail->next = topush;
	topush = qTail;
}

extern struct PCBNode* popTTYQHead(struct TTYQueue* qHead)
{
	struct TTYQueue* temp = qHead;
	qHead = temp->next;
	return temp->proc;
}

/* Physical page node util */
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
		TracePrintf(level, "%d: valid(%d), pfn(%d)\n", index, UserPageTable[index].valid, UserPageTable[index].pfn);
	}
}


