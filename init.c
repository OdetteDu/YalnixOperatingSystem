#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <stdio.h>
//int a = 5, c = 15;

int main()
{
//	while(1)
//	{
//		TracePrintf(0, "*****Init****\nMy PID is: %d.\n", GetPid());
//	  //printf("I'm in init!\n");
//	}
//		int b = 10, d = 20;
//		int i;

//		TracePrintf(2, "init!\n");
//		TracePrintf(2, "init: main %p\n", main);
//
//		for (i = 0; i < 20; i++)
//		TracePrintf(2, "pid %d\n", GetPid());


//	while(1)
//	{
//		  Pause();
void *currbreak;
    char *new;

    currbreak = sbrk(0);

    fprintf(stderr, "sbrk(0) = %p\n", currbreak);

    currbreak = (void *)UP_TO_PAGE(currbreak);
    currbreak++;
    currbreak = (void *)UP_TO_PAGE(currbreak);

    if (Brk(currbreak)) {
fprintf(stderr, "Brk %p returned error\n", currbreak);
Exit(1);
    }

    currbreak++;
    currbreak = (void *)UP_TO_PAGE(currbreak);

    if (Brk(currbreak)) {
fprintf(stderr, "Brk %p returned error\n", currbreak);
Exit(1);
    }

    new = malloc(10000);

    Exit(0);//	
}
