#include "io.h"
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
    int d;
    char* curHostName = "lunar";
    char* curUserName = "root";
    char* curDirName = "/";
    char str[100];
    while (1) {
         char *p;
         if (0) {
             printf("[%s@%s %s]# ", curHostName, curUserName, curDirName);           }
         else {
             printf("[%s@%s %s]# ", curHostName, curUserName, curDirName);
             fgets(str, 99, stdin);
             printf("command: %s\n", str);
         }
     }

    return 0;
}
