#include "userlib/syscall.h"
#define SIZE 1024

int main() {
    char tab[SIZE];

    n_printf("Testing page replacement...\n");

    int i;
    for (i = 0; i < SIZE; i++) {
        tab[i] = i;
    }

    n_printf("Done.\n");

    return 0;
}
