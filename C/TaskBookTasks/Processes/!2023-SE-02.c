#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>

pid_t pids[1024];

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(26, "Failed to read");
    }

    return r;
}

void killAllChildProcesses(int pidsCount) {
    for (int i = 0; i < pidsCount; i++) {
        if (kill(pids[i], SIGTERM) < 0) {
            err(26, "Failed to kill process");
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc == 1) {
        exit(26);
    }

    int pd[2];
    if (pipe(pd) < 0) {
        err(26, "Failed to pipe");
    }

    // Create a new process for each command
    for (int i = 1; i < argc; i++) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            err(26, "Failed to fork");
        }

        pids[i - 1] = child_pid;

        if (child_pid == 0) {
            close(pd[0]);

            if (dup2(pd[1], 1) < 0) {
                err(26, "Failed to dup2");
            }

            close(pd[1]);

            if (execlp(argv[i], argv[i], (void*)NULL) < 0) {
                err(26, "Failed to exec command %s", argv[i]);
            }
        }

        close(pd[1]);

        if (dup2(pd[0], 0) < 0) {
            err(26, "Failed to dup2");
        }

        close(pd[0]);

        const char* pattern = "found it!";
        char ch;
        int currIndex = 0;
        while (asserted_read(pd[0], &ch, sizeof(ch)) > 0) {
            if (ch == pattern[currIndex]) {
                currIndex++;
            }
            else {
                currIndex = 0;
            }

            // Check if the command output is the exact sentence
            if ((size_t)currIndex == strlen(pattern)) {
                killAllChildProcesses(i);
                exit(0);
            }
        }

        int status;
        if (wait(&status) < 0) {
            err(26, "Failed to wait for child process");
        }

        if (!WIFEXITED(status)) {
            err(26, "Command didn't exit successfully");
        }
    }
    exit(1);
}
