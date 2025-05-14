#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>

int asserted_write(int fd, const void* buffer, int size) {
    int w = write(fd, buffer, size);
    if (w < 0) {
        err(1, "Failed to write to %d", fd);
    }
    return w;
}

int asserted_read(int fd, void* buffer, int size) {
    int r = read(fd, buffer, size);
    if (r < 0) {
        err(1, "Failed to read from %d", fd);
    }
    return r;
}

int asserted_open(const char* filename, int mode, int* options) {
    int fd;
    if (options != NULL) {
        fd = open(filename, mode, *options);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(1, "Failed to open file %s", filename);
    }

    return fd;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int fdInput = asserted_open(argv[1], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    int bytesCount;
    uint16_t currByte;

    while ((bytesCount = asserted_read(fdInput, &currByte, sizeof(currByte))) == sizeof(currByte)) {
        uint8_t newByte = 0;
        uint16_t mask = 3;

        for (int i = 14; i >= 0; i -= 2) {
            uint8_t bits = (currByte >> i) & mask;
            if (bits == 2) {
                // Encoded 10, original bit 1
                newByte = (newByte << 1) | 1;
            }
            else if (bits == 1) {
                // Encoded 01, original bit 0
                newByte = (newByte << 1) | 0;
            }
            else {
                err(3, "Invalid encoded bits at position %d", i);
            }
        }

        asserted_write(fdOutput, &newByte, sizeof(newByte));
    }

    if (bytesCount < 0) {
        err(2, "Failed to read from file");
    }

    close(fdInput);
    close(fdOutput);
    return 0;
}
