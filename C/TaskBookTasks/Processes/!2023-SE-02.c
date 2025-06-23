// First solution
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
                pids[i] = 0;
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

// Second solution
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/wait.h>

pid_t pds[256] = { 0 };

pid_t asserted_fork(void) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        err(1, "Failed to fork");
    }

    return child_pid;
}

void asserted_pipe(int pd[2]) {
    if (pipe(pd) < 0) {
        err(26, "Failed to pipe");
    }
}

void asserted_dup2(int srcFd, int dstFd) {
    if (dup2(srcFd, dstFd) < 0) {
        err(26, "Failed to dup2");
    }
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(26, "Failed to read");
    }

    return r;
}

void closeAllRunningProcesses(int size) {
    for (int i = 0; i < size; i++) {
        if (kill(pds[i], SIGTERM) < 0) {
            err(26, "Failed to kill process");
        }
        wait(NULL);
    }
}

int main(int argc, char* argv[]) {

    // We loop through all args (programs) and create a process for each and then execute the given command
    for (int i = 1; i < argc; i++) {
        int pd[2];
        asserted_pipe(pd);

        pid_t child_pid = asserted_fork();
        pds[i - 1] = child_pid;

        if (child_pid == 0) {
            close(pd[0]);
            asserted_dup2(pd[1], 1);
            close(pd[1]);

            if (execlp(argv[i], argv[i], (void*)NULL) < 0) {
                err(26, "Failed to execute command %s", argv[i]);
            }
        }

        // We check if the output of the command was "found it!"
        close(pd[1]);
        char output[1024];
        int size = 0;
        char ch;
        while (asserted_read(pd[0], &ch, sizeof(ch)) > 0) {
            output[size++] = ch;
        }

        output[size] = '\0';

        if (strcmp(output, "found it!") == 0) {
            pds[i] = 0;
            closeAllRunningProcesses(argc);
            exit(0);
        }
    }

    // Wait for all processes to finish
    for (int i = 1; i < argc; i++) {
        int status;
        if (wait(&status) < 0) {
            err(26, "Failed to wait for child process");
        }

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                err(26, "Process exited with status != 0");
            }
        }
        else {
            err(26, "Process didn't exited");
        }
    }
    exit(1);
}