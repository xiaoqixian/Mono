#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    int d;
    char s[20];
};

int main() {
    printf("struct: %zx\n", sizeof(struct S));
    struct S s;
    printf("s: %zx\n", sizeof(s));
    return 0;
}

