#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    // Command: find -type f -printf "%T@ %f\n" | sort -n | head -n1 | cut -d '

    if (argc != 2) {
        errx(1, "Expected 1 arg");
    }

    // Executing command find
    int pd1[2];
    if (pipe(pd1) == -1) {
        err(2, "Failed to pipe");
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd1[0]);

        if (dup2(pd1[1], 1) == -1) {
            err(2, "Failed to dup2");
        }

        if (execlp("find", "find", argv[1], "-mindepth", "1", "-type", "f", "-printf", "%T@ %f\n", (void*)NULL) == -1) {
            err(2, "Failed to execute command find");
        }
    }
    else {
        close(pd1[1]);
    }

    // Executing command sort -n
    int status1;
    if (wait(&status1) == -1) {
        err(2, "Failed to wait for child process");
    }

    int pd2[2];
    if (pipe(pd2) == -1) {
        err(2, "Failed to pipe");
    }

    child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd2[0]);

        if (dup2(pd1[0], 0) == -1) {
            err(2, "Failed to dup2");
        }

        if (dup2(pd2[1], 1) == -1) {
            err(2, "Failed to dup2");
        }

        if (execlp("sort", "sort", "-nr", (void*)NULL) == -1) {
            err(2, "Failed to execute command sort");
        }
    }
    else {
        close(pd2[1]);
        close(pd1[0]);
    }

    // Executing command head -n
    int status2;
    if (wait(&status2) == -1) {
        err(2, "Failed to wait for child process");
    }

    int pd3[2];
    if (pipe(pd3) == -1) {
        err(2, "Failed to pipe");
    }

    child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd3[0]);

        if (dup2(pd2[0], 0) == -1) {
            err(2, "Failed to dup2");
        }

        if (dup2(pd3[1], 1) == -1) {
            err(2, "Failed to dup2");
        }

        if (execlp("head", "head", "-n1", (void*)NULL) == -1) {
            err(2, "Failed to execute command head");
        }
    }
    else {
        close(pd3[1]);
        close(pd2[0]);
    }

    // Executing command cut -f
    int status3;
    if (wait(&status3) == -1) {
        err(2, "Failed to wait for child process");
    }

    if (dup2(pd3[0], 0) == -1) {
        err(2, "Failed to dup2");
    }

    if (execlp("cut", "cut", "-f2", "-d", " ", (void*)NULL) == -1) {
        err(2, "Failed to execute command cut");
    }
}