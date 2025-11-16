#include <stdio.h>
#include <unistd.h>

int main(void) {
    uid_t uid = getuid();
    printf("User ID: %d\n", uid);
    return 0;
}
