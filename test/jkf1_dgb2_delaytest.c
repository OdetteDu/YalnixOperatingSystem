#include <stdio.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
    int delay_ticks;

    if (argc == 2) {
        delay_ticks = atoi(argv[1]);
    } else {
        delay_ticks = 5;
    }

    printf("Delaying process %d for %d ticks.\n", GetPid(), delay_ticks);
    Delay(delay_ticks);
    printf("Delay finished!\n");

    return 0;
}
