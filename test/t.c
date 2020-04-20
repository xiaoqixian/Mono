#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    char s[20];
};

void print(char* str) {
    write(1, str, strlen(str));
}

int main() {
    char s[20] = {
        0
    };
    free(&s);
    return 0;
}
