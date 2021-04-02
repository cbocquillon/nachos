#include "userlib/syscall.h"
#define RUNTIME 8
#define LENGTH 8

int main() {
    int i;
    char buff[LENGTH];
    for (i = 0; i < RUNTIME; i++) {
        TtySend("pong");
        TtyReceive(buff, LENGTH);
        n_printf("I received a %s !\n", buff);
    }

    return 0;
}
