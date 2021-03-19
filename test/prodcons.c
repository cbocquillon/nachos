#include "userlib/syscall.h"
#define N 16
#define RUNTIME 32

static SemId mutex, empty_slots, full_slots;
static char buffer[N];
static int prod_index = 0;
static int cons_index = 0;

void producer() {
    int i;
    for (i = 0; i < RUNTIME; i++) {
        P(empty_slots);
        P(mutex);

        buffer[prod_index] = i;
        n_printf("Test %d\n", i);
        prod_index = (prod_index+1) % N;

        V(mutex);
        V(full_slots);
    }
}

void consumer() {
    int i;
    for (i = 0; i < RUNTIME; i++) {
        P(full_slots);
        P(mutex);

        char value = buffer[cons_index];
        n_printf("Test consumer %d\n", value);
        cons_index = (cons_index+1) % N;

        V(mutex);
        V(empty_slots);
    }
}

int main() {
    mutex = SemCreate("mutex", 1);
    empty_slots = SemCreate("empty", N);
    full_slots = SemCreate("full", 0);
    ThreadId prod = threadCreate("producer", producer);
    ThreadId cons = threadCreate("consumer", consumer);
    Join(prod);
    Join(cons);

    return 0;
}
