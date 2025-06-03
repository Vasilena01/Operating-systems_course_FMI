#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>

// I have left the comments for testing and better understanding
pid_t pids[10] = { 0 };

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, perms);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file");
    }

    return fd;
}

pid_t asserted_fork(void) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        err(2, "Failed to fork process");
    }

    return child_pid;
}

void asserted_dup2(int new, int old) {
    if (dup2(new, old) < 0) {
        err(2, "Failed to dup2");
    }
}

pid_t create_new_child(const char* command, int fd_err) {
    pid_t child_pid = asserted_fork();

    if (child_pid == 0) {
        // We redirect the output of the command to go to /dev/null automaticaly
        asserted_dup2(fd_err, 1);

        if (execlp(command, command, (void*)NULL) < 0) {
            err(2, "Failed to exec command %s", command);
        }
    }

    return child_pid;
}

int getIndexFromPid(pid_t current_childPid) {
    for (int i = 0; i < 10; i++) {
        if (pids[i] == current_childPid) {
            return i;
        }
    }

    return -1;
}

void killAllAliveProcesses(void) {
    for (int i = 0; i < 10; i++) {
        if (pids[i] == 0) {
            continue;
        }

        printf("Processes to kill %d\n", pids[i]);
        if (kill(pids[i], SIGTERM) < 0) {
            err(4, "Failed to kill process %d", pids[i]);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1 || argc > 11) {
        errx(1, "Expected between 1 and 10 args");
    }

    // Create argc - 1 count of processes
    int fd_err = asserted_open("/dev/null", O_WRONLY, NULL);

    for (int i = 1; i < argc; i++) {
        pids[i - 1] = create_new_child(argv[i], fd_err);
        printf("New child pid %d \n", pids[i - 1]);
    }

    // All of the children will be waited here
    int status;
    pid_t current_childPid;

    while ((current_childPid = wait(&status)) > 0) {
        int index = getIndexFromPid(current_childPid);
        if (index < 0) {
            err(3, "Couldn't find index for this pid");
        }

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                //  We set the pid to be neutral, so we shouldn't wait it
                printf("Ok process, before pid is 0 %d \n", pids[index]);
                pids[index] = 0;
                printf("Ok process, after pid is 0 %d\n", pids[index]);
            }
            else {
                if (index < 0) {
                    err(3, "Couldn't find index for this pid");
                }

                pids[index] = create_new_child(argv[index], fd_err);
            }
        }
        else {
            if (WIFSIGNALED(status)) {
                printf("not Ok process, before pid is 0 %d\n", pids[index]);
                pids[index] = 0;
                printf("not Ok process, after pid is 0 %d\n", pids[index]);
                killAllAliveProcesses();
                exit(index + 1);
                break;
            }
        }
    }

    // Final wait for all processes to finish in order to avoid zombies
    for (int i = 0; i < 10; i++) {
        if (pids[i] == 0) {
            continue;
        }

        if (wait(&status) < 0) {
            err(5, "Failed to wait for last children");
        }
    }
}