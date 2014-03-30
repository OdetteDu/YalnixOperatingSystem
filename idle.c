#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	while(1)
	{	
		printf("I'm in idle now!\n");
		Pause();
		//Exit(99);
	}
}
