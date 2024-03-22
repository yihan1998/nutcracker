#include <stdio.h>

__attribute__((constructor))
void my_init_function() {
    printf("My initialization function\n");
}

void regular_function() {
    printf("A regular function\n");
}
