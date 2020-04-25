#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct S {
    int d;
};

int main() {
    char str[] = "mkdir c";
    char buf[10];
    sscanf(str, "%s", buf);
    printf("s: %s\n", buf);
    char buf2[10];
    sscanf(str, "%s%s", buf, buf2);
    printf("s1: %s\n", buf);
    printf("s2: %s\n", buf2);
    return 0;
}

int sd(int df) {
    return 0;
}
