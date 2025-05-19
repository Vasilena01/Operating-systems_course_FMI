#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, perms);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open");
    }

    return fd;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write");
    }

    return w;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(3, "Failed to read");
    }

    return r;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        err(1, "Expected 2 args");
    }

    int pd[2];
    if (pipe(pd) == -1) {
        err(3, "Failed to pipe");
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        err(3, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd[0]);

        if (dup2(pd[1], 1) == -1) {
            err(4, "Failed to dup2");
        }

        if (execlp("cat", "cat", argv[1], (char*)NULL) < 0) {
            err(4, "Failed to exec cat command");
        }
    }

    close(pd[1]);
    int status;
    if (wait(&status) < 0) {
        err(4, "Failed to wait for child process");
    }

    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, &perms);
    uint8_t byte;
    int bytesRead;
    while ((bytesRead = asserted_read(pd[0], &byte, sizeof(byte))) > 0) {
        if (byte == 0x7D) {
            uint8_t nextByte;
            asserted_read(pd[0], &nextByte, sizeof(nextByte));
            uint8_t encodedByte = nextByte ^ 0x20;
            if (encodedByte == 0x00 || encodedByte == 0xFF ||
                encodedByte == 0x55 || encodedByte == 0x7D) {
                asserted_write(fdOutput, &encodedByte, sizeof(encodedByte));
            }
            else {
                err(5, "Invalid file content");
            }
        }
        else if (byte == 0x55) {
            continue;
        }
        else {
            err(5, "Invalid file content");
        }
    }

    if (bytesRead < 0) {
        err(5, "Failed to read bytes from pipe");
    }

    close(fdOutput);
}