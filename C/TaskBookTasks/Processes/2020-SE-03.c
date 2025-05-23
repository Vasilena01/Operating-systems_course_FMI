// First solution - with max 8 pipes, for each child - 1 pipe (haven't tested it, but maybe it's ok)
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>

int fds[2];

typedef struct {
    char filename[8];
    uint32_t offset;
    uint32_t length;
} FileData;

void close_all(void) {
    for (int i = 0; i < 2; i++) {
        if (fds[i] >= 0) {
            close(fds[i]);
        }
        else {
            err(9, "Failed to close fd");
        }
    }
}

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
        close_all();
        err(2, "Failed to read");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        close_all();
        err(2, "Failed to write");
    }

    return w;
}

int asserted_lseek(int fd, off_t offset, int whence) {
    off_t ls = lseek(fd, offset, whence);

    if (ls < 0) {
        close_all();
        err(2, "Failed to lseek");
    }

    return ls;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        err(1, "Expected 1 arg");
    }

    fds[0] = asserted_open(argv[1], O_RDONLY, NULL);
    struct stat st;

    if (fstat(fds[0], &st) < 0) {
        err(3, "Failed to fstat");
    }

    FileData fileData;
    if (st.st_size % sizeof(fileData) != 0 || (long unsigned int)st.st_size > (s                                                                                            izeof(fileData) * 8)) {
        err(4, "File doenst have the required size");
    }

    size_t structCount = st.st_size / sizeof(fileData);

    if (structCount > 8) {
        err(4, "File must contain max 8 structs");
    }

    // Creating a pipe to pass info between the input and output files
    int pd[8][2];

    for (size_t i = 0; i < structCount; i++) {
        FileData currData;
        asserted_read(fds[0], &currData, sizeof(currData));

        if (pipe(pd[i]) == -1) {
            err(8, "Pipe failed");
        }

        pid_t child_pid = fork();
        if (child_pid == -1) {
            close_all();
            err(5, "Failed to fork");
        }

        if (child_pid == 0) {
            close(pd[i][0]);
            fds[1] = asserted_open(currData.filename, O_RDONLY, NULL);
            asserted_lseek(fds[1], currData.offset * sizeof(uint16_t), SEEK_SET);
            uint16_t res = 0;
            uint16_t num;

            for (int j = 0; j < (int)currData.length; j++) {
                asserted_read(fds[1], (void*)&num, sizeof(num));
                res = res ^ num;
            }

            // Pass the current res to the pipe, so later to xor all of them
            asserted_write(pd[i][1], (void*)&res, sizeof(res));
            exit(0);
        }

        close(pd[i][1]);
    }

    // Read input from pipe and xor all of it
    for (size_t i = 0; i < structCount; i++) {
        int status;
        if (wait(&status) < 0) {
            err(9, "Failed to wait child");
        }
    }

    uint16_t finalResult = 0;
    for (size_t i = 0; i < structCount; i++) {
        uint16_t num;
        asserted_read(pd[i][0], (void*)&num, sizeof(num));
    }

    dprintf(1, "%dB", finalResult);

    close_all();
}

// Second solution - with just one pipe, but maybe it will lead to deadlock...
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>

int fds[2];

typedef struct {
    char filename[8];
    uint32_t offset;
    uint32_t length;
} FileData;

void close_all(void) {
    for (int i = 0; i < 2; i++) {
        if (fds[i] >= 0) {
            close(fds[i]);
        }
        else {
            err(9, "Failed to close fd");
        }
    }
}

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, *perms);
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
        close_all();
        err(2, "Failed to read");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        close_all();
        err(2, "Failed to write");
    }

    return w;
}

int asserted_lseek(int fd, off_t offset, int whence) {
    off_t ls = lseek(fd, offset, whence);

    if (ls < 0) {
        close_all();
        err(2, "Failed to lseek");
    }

    return ls;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        err(1, "Expected 1 arg");
    }

    fds[0] = asserted_open(argv[1], O_RDONLY, NULL);

    struct stat st;
    if (fstat(fds[0], &st) < 0) {
        err(3, "Failed to fstat");
    }

    FileData fileData;
    if (st.st_size % sizeof(fileData) != 0 || (long unsigned int)st.st_size > (sizeof(fileData) * 8)) {
        err(4, "File doesn't have the required size");
    }

    size_t structCount = st.st_size / sizeof(fileData);

    if (structCount > 8) {
        err(4, "File must contain max 8 structs");
    }

    int pd[2];
    if (pipe(pd) == -1) {
        err(8, "Pipe failed");
    }

    for (size_t i = 0; i < structCount; i++) {
        FileData currData;
        asserted_read(fds[0], &currData, sizeof(currData));

        pid_t child_pid = fork();
        if (child_pid == -1) {
            close_all();
            err(5, "Failed to fork");
        }

        if (child_pid == 0) {
            close(pd[0]);
            fds[1] = asserted_open(currData.filename, O_RDONLY, NULL);
            asserted_lseek(fds[1], currData.offset * sizeof(uint16_t), SEEK_SET);

            uint16_t res = 0x0000;
            uint16_t num;

            for (int j = 0; j < (int)currData.length; j++) {
                asserted_read(fds[1], (void*)&num, sizeof(num));
                res = res ^ num;
            }

            asserted_write(pd[1], (void*)&res, sizeof(res));
            exit(0);
        }
    }

    close(pd[1]);

    uint16_t num;
    uint16_t finalResult = 0x0000;
    int bytesCount;

    while ((bytesCount = asserted_read(pd[0], (void*)&num, sizeof(num))) > 0) {
        finalResult = finalResult ^ num;
    }

    if (bytesCount < 0) {
        err(10, "Failed to read from pipe");
    }

    dprintf(1, "%dB\n", finalResult);

    close_all();
    return 0;
}