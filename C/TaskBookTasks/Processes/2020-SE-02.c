// First solution
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
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
        err(2, "Failed to open file");
    }

    return fd;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Failed to read from file");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write to file");
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int perms = S_IXUSR | S_IRUSR | S_IWUSR;
    int fdOutput = asserted_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &perms);

    // In order the reading to happen via an external command we will exec cat for the inputFile
    int pd[2];
    if (pipe(pd) < 0) {
        err(3, "Failed to pipe");
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        err(3, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd[0]);

        if (dup2(pd[1], 1) < 0) {
            err(4, "Failed to dup2");
        }

        close(pd[1]);

        if (execlp("cat", "cat", argv[1], (void*)NULL) < 0) {
            err(4, "Failed to exec cat command");
        }
    }

    close(pd[1]);

    // Read byte by byte from pipe, make checks and write it to output file
    uint8_t curr;

    while (asserted_read(pd[0], &curr, sizeof(curr)) > 0) {
        if (curr == 0x55) {
            continue;
        }
        else if (curr == 0x7D) {
            // So we read the next byte, because it's a special one
            asserted_read(pd[0], &curr, sizeof(curr));
            curr = curr ^ 0x20;
            //if (curr != 0x00 || curr != 0xFF || curr != 0x55 || curr != 0x7D) {
            //    errx(5, "Invalid input file data");
            //}
        }
        asserted_write(fdOutput, &curr, sizeof(curr));
    }

    close(fdOutput);
}

// Second solution
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