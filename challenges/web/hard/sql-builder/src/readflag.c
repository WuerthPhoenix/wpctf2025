#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int main(){
    // print /flag with setuid 0
    setuid(0);
    char random1[10], random2[10];
    char input1[10], input2[10];

    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, random1, sizeof(random1));
    read(fd, random2, sizeof(random2));
    close(fd);

    printf("Generated strings: %s, %s\n", random1, random2);

    printf("Enter first string: ");
    fgets(input1, sizeof(input1), stdin);
    printf("Enter second string: ");
    fgets(input2, sizeof(input2), stdin);

    if (strcmp(input1, random1) != 0 || strcmp(input2, random2) != 0) {
        exit(1);
    }

    FILE *f = fopen("/flag.txt", "r");
    char flag[100];
    fgets(flag, 100, f);
    printf("%s\n", flag);
    fclose(f);
}