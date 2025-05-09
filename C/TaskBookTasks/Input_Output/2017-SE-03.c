#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

typedef struct {
    uint16_t offset;
    uint8_t originalByte;
    uint8_t newByte;
} Patch;

int main(int argc, char* argv[]) {

    if (argc != 4) {
        errx(1, "Expected 3 args");
    }

    int fdPatch = open(argv[1], O_RDONLY);
    if (fdPatch == -1) {
        err(2, "Failed to open file %s", argv[1]);
    }

    int fd1 = open(argv[2], O_RDONLY);
    if (fd1 == -1) {
        err(2, "Failed to open file %s", argv[2]);
    }

    int fd2 = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd2 == -1) {
        err(2, "Failed to open file %s", argv[3]);
    }

    Patch p;
    int bytesCount;
    uint16_t lastAccesedIndx = 0;

    while ((bytesCount = read(fdPatch, &p, sizeof(p))) == sizeof(p)) {

        if (lseek(fd1, lastAccesedIndx, SEEK_SET) == -1) {
            err(3, "Failed to lseek fd1");
        }

        // Read all bytes before offset and write them to fd2
        for (uint16_t i = lastAccesedIndx; i < p.offset; i++) {
            uint8_t curr;

            if (read(fd1, &curr, sizeof(curr)) != sizeof(curr)) {
                err(3, "Failed to read from file %s", argv[2]);
            }

            if (write(fd2, &curr, sizeof(curr)) != sizeof(curr)) {
                err(3, "Failed to write to file %s", argv[3]);
            }
        }

        // Read offsetByte
        if (lseek(fd1, p.offset, SEEK_SET) == -1) {
            err(3, "Failed to lseek fd1");
        }

        uint8_t byteInF1;
        if (read(fd1, &byteInF1, sizeof(byteInF1)) != sizeof(byteInF1)) {
            err(3, "Failed to read from file %s", argv[2]);
        }

        // Compare offset byte with original byte
        if (byteInF1 != p.originalByte) {
            err(4, "Byte at position offset is not equal to original byte in patch");
        }

        if (write(fd2, &p.newByte, sizeof(p.newByte)) != sizeof(p.newByte)) {
            err(3, "Failed to write to file %s", argv[3]);
        }

        lastAccesedIndx = p.offset + 1;
    }

    if (bytesCount < 0) {
        err(3, "Couldn't read struct patch from file");
    }

    // Copy remaining bytes from fd1 to fd2, after finishing all patches
    uint8_t buff[4096];
    int bytesRead;

    if (lseek(fd1, lastAccesedIndx, SEEK_SET) == -1) {
        err(3, "Failed to lseek fd1");
    }

    while ((bytesRead = read(fd1, &buff, sizeof(buff))) > 0) {
        if (write(fd2, &buff, bytesRead) != bytesRead) {
            err(3, "Failed to write bytes to fd2");
        }
    }

    if (bytesRead < 0) {
        err(3, "Failed to read remaining bytes from fd1");
    }

    close(fdPatch);
    close(fd1);
    close(fd2);
}