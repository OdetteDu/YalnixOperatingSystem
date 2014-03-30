#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main()
{
	while(1)
	{
		TracePrintf(0, "*****Init****\nMy PID is: %d.\n", GetPid());
	  //printf("I'm in init!\n");
	}
}
