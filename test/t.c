#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    char s;
    int d;
};

void print(char* str) {
    write(1, str, strlen(str));
}

int main() {
    print("hello world");
    return 0;
}
