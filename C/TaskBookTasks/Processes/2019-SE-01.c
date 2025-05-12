#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <time.h>
#include <sys/wait.h>

int asserted_open(const char* filename, int status, int* perms) {

    int fd;
    if (perms != NULL) {
        fd = open(filename, status, *perms);
    }
    else {
        fd = open(filename, status);
    }

    if (fd < 0) {
        err(2, "Couldn't open file %s", filename);
    }

    return fd;
}

int asserted_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Couldn'r write to file");
    }

    return w;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        errx(1, "Expected min 2 args");
    }
    char* endptr;
    int minDuration = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0') {
        errx(1, "Invalid number %s", argv[1]);
    }

    if (minDuration < 0 || minDuration > 9) {
        err(2, "First arg should be between 0-9");
    }

    const char* command = argv[2];
    char** command_args = argv + 2;

    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fd = asserted_open("run.log", O_CREAT | O_TRUNC, &perms);
    close(fd);

    int timesFailed = 0;
    bool lastTimeFailed = false;

    while (true) {
        pid_t child_pid = fork();

        if (child_pid == -1) {
            err(3, "Failed to fork");
        }

        if (child_pid == 0) {
            if (execvp(command, command_args) < 0) {
                err(4, "Failed to exit command");
            }
        }

        int status;
        time_t start = time(NULL);
        if (wait(&status) < 0) {
            err(5, "Couldn't wait for child to finish");
        }
        time_t end = time(NULL);
        int exit_code;

        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        }
        else {
            exit_code = 129;
        }

        fd = asserted_open("run.log", O_WRONLY | O_APPEND, NULL);

        char startStr[32];
        char endStr[32];
        char codeStr[32];

        snprintf(startStr, sizeof(startStr), "%ld", start);
        snprintf(endStr, sizeof(endStr), "%ld", end);
        snprintf(codeStr, sizeof(codeStr), "%d", exit_code);

        asserted_write(fd, startStr, strlen(startStr));
        asserted_write(fd, " ", 1);
        asserted_write(fd, endStr, strlen(endStr));
        asserted_write(fd, " ", 1);
        asserted_write(fd, codeStr, strlen(codeStr));
        asserted_write(fd, "\n", 1);

        close(fd);

        double duration = end - start;

        if (exit_code != 0 && duration < minDuration) {
            timesFailed++;
            lastTimeFailed = true;
        }
        else {
            timesFailed = 0;
            lastTimeFailed = false;
        }

        if (timesFailed == 2 && lastTimeFailed) {
            break;
        }

        close(fd);
    }
}
