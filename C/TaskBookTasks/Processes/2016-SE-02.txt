#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <sys/wait.h>

int main(void) {
    while (true) {
        write(1, "Enter a command: \n", 19);

        char cmd[100];
        int bytesCount;

        if ((bytesCount = read(0, cmd, sizeof(cmd))) < 0) {
            err(1, "Failed to read command");
        }

        if (bytesCount > 0 && cmd[bytesCount - 1] == '\n') {
            cmd[bytesCount - 1] = '\0';
        }

        if (strcmp(cmd, "exit") == 0) {
            exit(0);
        }

        pid_t child_pid = fork();

        if (child_pid == -1) {
            err(2, "Failed to fork");
        }

        if (child_pid == 0) {
            if (execlp(cmd, cmd, (char*)NULL) == -1) {
                err(3, "Failed to execute cmd %s", cmd);
            }
        } else {
            wait(NULL);
        }
    }
}