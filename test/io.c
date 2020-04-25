#include <stdio.h>
#include <stdlib.h>

struct S {
    char s[4];
    int d;
    int t;
};

struct C {
    char c[8];
    int r;
};

int main() {
    system("reset");
    struct C c;
    c.c[2] = 'c';
    struct C* s = &c;
    printf("%c\n",s->c[2]);
    return 0;
}
