#include <stdio.h>
#include <stdlib.h>

void main(void)
{
    void* ptr1 = malloc(10);
    void* ptr2 = malloc(20);
    void* ptr3 = malloc(30);
    void* ptr4 = malloc(40);
    void* ptr5 = ptr1;

    free(ptr1);
    free(ptr2);
    free(ptr4);
    free(ptr5);
}
