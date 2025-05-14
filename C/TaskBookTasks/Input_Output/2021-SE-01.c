#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

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
        err(2, "Failed to read");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write");
    }

    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        err(1, "Expected 2 args to be passed");
    }

    int fdInput = asserted_open(argv[1], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, &perms);

    int bytesCount;
    uint8_t currByte;

    while ((bytesCount = asserted_read(fdInput, &currByte, sizeof(currByte))) > 0) {
        uint16_t newByte = 0;

        for (int i = 7; i >= 0; i--) {
            uint8_t bit = (currByte >> i) & 1;
            uint16_t newBit;

            if (bit == 0) {
                newBit = 1 << ((7 - i) * 2);  // Encodes 0 as 01
            }
            else {
                newBit = 2 << ((7 - i) * 2);  // Encodes 1 as 10
            }

            newByte |= newBit;
        }

        asserted_write(fdOutput, &newByte, sizeof(newByte));
    }

    if (bytesCount < 0) {
        err(3, "Couldn't read bytes");
    }

    close(fdInput);
    close(fdOutput);

    return 0;
}
