#include <stdio.h>
#include <string.h>

void print_cpu_info(char* value) {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo == NULL) {
        printf("Unable to open /proc/cpuinfo\n");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), cpuinfo)) {
        char* ptr = strchr(line, '\n');
        *ptr = '\0';

        if (strncmp(line, "processor", 9) == 0) {
            printf("\n%s\n", line);
            continue;
        }
        if (value != NULL && strncmp(line, value, strlen(value)) != 0) {
            continue;
        }
        printf("%s\n", line);
    }
    fclose(cpuinfo);
}

void print_mem_info(char* line) {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL) {
        printf("Unable to open /proc/meminfo\n");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), meminfo)) {
        char* ptr = strchr(buffer, '\n');
        *ptr = '\0';

        if (line != NULL && strncmp(buffer, line, strlen(line)) != 0) {
            continue;
        }
        printf("%s\n", buffer);
    }
    fclose(meminfo);
}

void print_disk_stats(char* disk) {
    FILE *diskstats = fopen("/proc/diskstats", "r");
    if (diskstats == NULL) {
        printf("Unable to open /proc/diskstats\n");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), diskstats)) {
        char* ptr = strchr(buffer, '\n');
        *ptr = '\0';

        int disk_name_start = -1;
        sscanf(buffer, "%*s %*s %n", &disk_name_start);
        if (disk_name_start == -1) {
            continue;
        }
        char *disk_name = buffer + disk_name_start;
        if (disk != NULL && strncmp(disk_name, disk, strlen(disk)) != 0) {
            continue;
        }

        printf("%s\n", buffer);
    }
    fclose(diskstats);
}
