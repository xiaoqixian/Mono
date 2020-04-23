#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct S {
    int d;
};

int* print(int nums[]) {
    return nums;
}

int main() {
    int nums[] = {
        0,2,3,4
    };
    int* num = print(nums);
    printf("%d\n", *num);
    printf("%d\n", nums[0]);
    return 0;
}
