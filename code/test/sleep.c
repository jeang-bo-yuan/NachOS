#include "syscall.h"

/**
 * test whether the Sleep system call works
*/

main() {
    int i;
    for (i = 1; i <= 5; ++i) {
        PrintInt(i);
        Sleep(1000000 * i);
    }
}