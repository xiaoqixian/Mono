#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    int d;
    char s[20];
};


int main() {
    struct S* s = (struct S*)malloc(2 * sizeof(struct S));
    free(s);
        return 0;
}

