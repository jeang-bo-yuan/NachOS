#include "syscall.h"

main() {
    int len;

    len = Print("Hello NachOS2024!\n");
    PrintInt(len);

    len = Print("Have a nice day at school!\n");
    PrintInt(len);

    len = Print("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n");
    PrintInt(len);
}