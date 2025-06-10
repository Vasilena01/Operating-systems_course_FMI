#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
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

typedef struct {
    int32_t magic; // 0x21494D46
    int32_t packet_count; // packets count in compressed file
    int64_t original_size; // size of the original result file
} Header;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int fdCompressed = asserted_open(argv[1], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdResult = asserted_open(argv[2], O_CREAT | O_TRUNC | O_RDWR, &perms);

    Header header;
    asserted_read(fdCompressed, &header, sizeof(header));
    if (header.magic != 0x21494D46) {
        err(3, "Invalid magic number in header");
    }

    for (int32_t i = 0; i < header.packet_count; i++) {
        int8_t firstByte;
        asserted_read(fdCompressed, &firstByte, sizeof(firstByte));

        // Check the first bit of the byte - based on big endian
        int8_t mask = 1;
        int8_t N = firstByte & ~(mask << 7);
        if (firstByte & (mask << 7)) {
            // If its 1
            int8_t nextByte;
            asserted_read(fdCompressed, &nextByte, sizeof(nextByte));
            for (int8_t j = 0; j < N + 1; j++) {
                asserted_write(fdResult, &nextByte, sizeof(nextByte));
            }
        }
        else {
            // If its 0 - write the rest N + 1 bytes to the result file
            int8_t curr;
            for (int8_t j = 0; j < N + 1; j++) {
                asserted_read(fdCompressed, &curr, sizeof(curr));
                asserted_write(fdResult, &curr, sizeof(curr));
            }
        }
    }

    struct stat stResult;
    if (fstat(fdResult, &stResult) < 0) {
        err(3, "Failed to fstat");
    }

    if (stResult.st_size != header.original_size) {
        err(6, "Result file doesn't have the correct size");
    }

    close(fdCompressed);
    close(fdResult);
}