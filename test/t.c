#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    int d;
    char s[20];
};

void sd(char path[]) {
    if (strlen(path) < 1) {
        return;
    }
    printf("%c\n", path[0]);
    char* sub_path = &path[1];
    sd(sub_path);
}

int main() {
    int res = access("tmp", F_OK);
    if (res == -1) {
        printf("dir not exits\n");
    }
        return 0;
}

