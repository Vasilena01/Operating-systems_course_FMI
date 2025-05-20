#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
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

    int fdInput = asserted_open(argv[1], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    uint8_t currByte;
    int bytesRead;

    while ((bytesRead = asserted_read(fdInput, &currByte, sizeof(currByte))) > 0) {

        if (currByte == 0x55) {
            uint8_t currCheckSum = 0;
            asserted_write(fdOutput, &currByte, sizeof(currByte));
            currCheckSum = currCheckSum ^ currByte;

            uint8_t N;
            asserted_read(fdInput, &N, sizeof(N));
            if (N < 3) {
                err(3, "N should be > 3");
            }
            asserted_write(fdOutput, &N, sizeof(N));
            currCheckSum = currCheckSum ^ N;

            uint8_t messageDataByte;
            for (uint8_t i = 2; i < N - 1; i++) {
                asserted_read(fdInput, &messageDataByte, sizeof(messageDataByte));
                asserted_write(fdOutput, &messageDataByte, sizeof(messageDataByte));
                currCheckSum = currCheckSum ^ messageDataByte;
            }

            uint8_t inputCheckSum;
            asserted_read(fdInput, &inputCheckSum, sizeof(inputCheckSum));
            if (inputCheckSum != currCheckSum) {
                err(4, "Input checksum isn't the expected value");
            }
            asserted_write(fdOutput, &currCheckSum, sizeof(currCheckSum));
        }
    }

    if (bytesRead < 0) {
        err(3, "Failed to read bytes from file");
    }

    close(fdInput);
    close(fdOutput);
}