#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "info.h"

void print_help_message() {
    printf("Available commands:\n");
    printf("  ?          - Show this help message\n");
    printf("  exit       - Exit the prompt\n");
    printf("  info       - Display system information\n");
    printf("  flag <pwd> - Show flag, if the password matches\n");
}

void print_system_info_help_message() {
    printf("Usage: info <option>\n");
    printf("Options:\n");
    printf("  cpu <?line>    - Show CPU information\n");
    printf("  memory <?line> - Show memory information\n");
    printf("  disk <?disk>   - Show disk information\n");
}

typedef struct {
    const char* name;
    void (*func)(char* arg);
} info_dispatch_t;

void print_system_info(char* info_command) {
    info_dispatch_t info_commands[] = {
        {"cpu", print_cpu_info},
        {"memory", print_mem_info},
        {"disk", print_disk_stats},
        {NULL, NULL}
    };

    for (int i = 0; info_commands[i].name != NULL; i++) {
        size_t name_len = strlen(info_commands[i].name);
        if (strncmp(info_command, info_commands[i].name, name_len) != 0) {
            continue;
        }

        char* arg = NULL;
        if (strlen(info_command) > name_len + 1) {
            arg = info_command + name_len + 1;
        }
        info_commands[i].func(arg);
        return;
    }

    printf("Unknown info command: %s\n", info_command);
}

void print_flag(const char* pwd) {
    const char* correct_pwd = PASSWORD;
    if (strcmp(pwd, correct_pwd) == 0) {
        printf("Congratulations! Here is your flag: %s\n", FLAG);
    } else {
        printf("Incorrect password. '");
        printf(pwd);
        printf("' Access denied.\n");
    }
}

int process_command(const char *command) {
    if (strcmp(command, "exit") == 0) {
        printf("Exiting the system information prompt. Goodbye!\n");
        return 0;
    } else if (strcmp(command, "?") == 0) {
        print_help_message();
    } else if (strncmp(command, "info", 4) == 0) {
        if (strlen(command) > 5) {
            print_system_info((char *)(command + 5));
        } else {
            print_system_info_help_message();
        }
    } else if (strncmp(command, "flag", 4) == 0) {
        if (strlen(command) > 5) {
            print_flag(command + 5);
        } else {
            printf("Usage: flag <password>\n");
        }
    } else {
        printf("Unknown command:");
        printf(command);
        printf("\n");
    }
    return 1;
}

void repl() {
    char input_buffer[64];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            break;
        }
        // Remove newline character
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        if (strlen(input_buffer) == 0) {
            continue;
        }

        if (process_command(input_buffer) == 0) {
            break;
        }
    }
}

int main(void) {
    printf("Welcome to your system information prompt!\n");
    printf("Type '?' for help or 'exit' to quit.\n");
    printf("Please enter your command:\n");
    repl();
    return 0;
}
