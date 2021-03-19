#include "userlib/syscall.h"
#define NB_THREADS 8
#define RUNTIME 5

static SemId mutex;
static char critical_value;

void work() {
    int i;
    for (i = 0; i < RUNTIME; i++) {
        P(mutex);
        critical_value++;
        V(mutex);
    }
}

int main() {
    int i;
    critical_value = 0;
    mutex = SemCreate("mutex", 1);
    ThreadId threads[NB_THREADS];

    for (i = 0; i < NB_THREADS; i++) {
        threads[i] = newThread("a worker", work, 0);
    }
    for (i = 0; i < NB_THREADS; i++) {
        Join(threads[i]);
    }
    return 0;
}
