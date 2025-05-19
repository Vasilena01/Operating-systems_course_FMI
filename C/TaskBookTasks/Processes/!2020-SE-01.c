//foo.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <err.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Expected 1 arg");
    }

    const char* filename = argv[1];
    int fifo = mkfifo("my_pipe", 0644);
    if (fifo == -1) {
        err(2, "Failed to make named pipe");
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        int fdFifo = open("my_pipe", O_WRONLY);
        if (fdFifo == -1) {
            err(2, "Failed to open fifo");
        }

        if (dup2(fdFifo, 1) == -1) {
            close(fdFifo);
            err(3, "Couldn't dup2");
        }

        if (execlp("cat", "cat", filename, (void*)NULL) == -1) {
            close(fdFifo);
            err(3, "Failed to execute cat");
        }
    }

    int status;
    if (wait(&status) == -1) {
        err(4, "Failed to wait for the child");
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            err(4, "Child exited with status != 0");
        }
    }
    else {
        err(4, "Failed to exit child process");
    }
}

//bar.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Expected 1 arg");
    }

    const char* command = argv[1];
    pid_t child_pid = fork();

    if (child_pid == -1) {
        err(2, "Failed to fork");
    }

    if (child_pid == 0) {
        int fdFifo = open("my_fifo", O_RDONLY);
        if (fdFifo < 0) {
            err(2, "Failed to open fifo");
        }

        if (dup2(fdFifo, 0) == -1) {
            close(fdFifo);
            err(3, "Failed to dup2");
        }

        if (execlp(command, command, (void*)NULL) == -1) {
            close(fdFifo);
            err(3, "Failed to exec command");
        }
    }

    int status;
    if (wait(&status) == -1) {
        err(4, "Failed to wait child ps");
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            err(5, "Child process exited with status != 0");
        }
    }
    else {
        err(5, "Child process terminated");
    }

    if (unlink("my_fifo") == -1) {
        err(6, "Failed to unlink named pipe");
    }
}
