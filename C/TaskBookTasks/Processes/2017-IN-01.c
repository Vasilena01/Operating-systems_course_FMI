#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>

int main(void) {

    // Command: cut -d : -f 7 /etc/passwd | sort | uniq -c | sort -n
    int pd1[2];
    if (pipe(pd1) == -1) {
        err(1, "Pipe failed");
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd1[0]);
        if (dup2(pd1[1], 1) == -1) {
            err(3, "Failed to dup");
        }

        if (execlp("cut", "cut", "-d", ":", "-f", "7", "/etc/passwd", (void*)NULL) == -1) {
            err(3, "Failed to exec cut command");
        }
    }
    else {
        close(pd1[1]);
    }

    int status;
    while (wait(&status) < 0) {
        err(4, "Failed to wait children processes");
    }

    int pd2[2];
    if (pipe(pd2) == -1) {
        err(1, "Pipe failed");
    }

    child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd2[0]);

        if (dup2(pd1[0], 0) == -1) {
            err(3, "Failed to dup2");
        }

        if (dup2(pd2[1], 1) == -1) {
            err(3, "Failed to dup2");
        }

        if (execlp("sort", "sort", (void*)NULL) == -1) {
            err(3, "Failed to execute sort command");
        }
    }
    else {
        close(pd2[1]);
    }

    while (wait(&status) < 0) {
        err(4, "Failed to wait children processes");
    }

    int pd3[2];
    if (pipe(pd3) == -1) {
        err(1, "Pipe failed");
    }

    child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd3[0]);

        if (dup2(pd2[0], 0) == -1) {
            err(3, "Failed to dup2");
        }

        if (dup2(pd3[1], 1) == -1) {
            err(3, "Failed to dup2");
        }

        if (execlp("uniq", "uniq", "-c", (void*)NULL) == -1) {
            err(3, "Failed to execute uniq -c command");
        }
    }
    else {
        close(pd3[1]);
    }

    close(pd1[0]);
    close(pd2[0]);
    while (wait(&status) < 0) {
        err(4, "Failed to wait children processes");
    }

    if (dup2(pd3[0], 0) == -1) {
        err(3, "Failed to dup2");
    }

    if (execlp("sort", "sort", "-n", (void*)NULL) == -1) {
        err(3, "Failed to execute sort -n command");
    }
}