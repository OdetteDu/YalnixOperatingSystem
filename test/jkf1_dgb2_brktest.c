#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define DATA_SIZE  1000
#define DATA_COUNT 100

typedef struct data {
    int id;
    int data [DATA_SIZE];
} data;

int main() {
    data * data_items[DATA_COUNT];
    int i;

    for (i = 0; i < DATA_COUNT; i++) {
        printf("Allocating data item %d. Current break is %p.\n", i, sbrk(0));
        
        if ((data_items[i] = malloc(sizeof(*data_items[i]))) == NULL) {
            printf("Mallow failed on item %d.\n", i);
            return -1;
        }

        data_items[i]->id = i;

        printf("Finished allocating data item %d. Current break is %p.\n", 
               i, sbrk(0));
    }

    for (i = DATA_COUNT - 1; i >= 0; i--) {
        printf("Freeing data item %d. Current break is %p.\n", i, sbrk(0));

        if (data_items[i]->id != i) {
            printf("Data item %d has bad id %d.\n", i, data_items[i]->id);
        }

        free(data_items[i]);

        printf("Finished freeing data item %d. Current break is %p.\n",
               i, sbrk(0));
    }

    return 0;
}
