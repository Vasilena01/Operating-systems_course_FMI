#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

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
        uint8_t mask = 1;

        for (int i = 0; i < 8; i++) {
            uint8_t bit = currByte & (mask << i);

            if (bit == 0) {
                newByte |= 1 << i * 2;
            }
            else {
                newByte |= 2 << i * 2;
            }
        }

        // To solve the problem with the endianess
        uint8_t* newBytePtr = (uint8_t*)&newByte;

        asserted_write(fdOutput, &newBytePtr[1], sizeof(uint8_t));
        asserted_write(fdOutput, &newBytePtr[0], sizeof(uint8_t));
    }

    if (bytesCount < 0) {
        err(3, "Couldn't read bytes");
    }

    close(fdInput);
    close(fdOutput);
}