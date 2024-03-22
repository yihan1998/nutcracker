#include <stdio.h>

#include "sid.h"

int main() {
    int sfd = sid_object__open_and_load("sid_test_prog.wasm");
    printf("sfd: %d\n", sfd);
    return 0;
}