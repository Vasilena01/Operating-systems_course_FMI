#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>

typedef struct {
    uint32_t magic;
    uint32_t count;
} NormalHeader;

typedef struct {
    uint32_t magic1;
    uint16_t magic2;
    uint16_t reserved;
    uint64_t count;
} ComparatorHeader;

typedef struct {
    uint16_t type; // 0 = < , 1 = >
    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t reserved3;
    uint32_t offset1;
    uint32_t offset2;
} ComparatorDataBlock;

void open_err(int fd) {
    if (fd == -1) {
        err(2, "Failed to open file");
    }
}

void read_err(int fd, void* toRead, size_t size) {
    if (read(fd, toRead, size) != size) {
        err(3, "Failed to read");
    }
}

void write_err(int fd, void* toWrite, size_t size) {
    if (write(fd, toWrite, size) != size) {
        err(3, "Failed to write");
    }
}

void swapAndSave(int fd1, uint64_t num1, uint64_t num2, uint32_t offset1, uint32_t offset2) {
    uint64_t tmp = num1;
    num1 = num2;
    num2 = tmp;

    if (lseek(fd1, offset1 * sizeof(uint64_t), SEEK_SET) == -1) {
        err(4, "Failed to lseek");
    }
    write_err(fd1, &num1, sizeof(num1));

    if (lseek(fd1, offset2 * sizeof(uint64_t), SEEK_SET) == -1) {
        err(4, "Failed to lseek");
    }
    write_err(fd1, &num2, sizeof(num2));
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int fd1 = open(argv[1], O_RDWR);
    open_err(fd1);

    int fd2 = open(argv[2], O_RDONLY);
    open_err(fd2);

    NormalHeader normalHeader;
    read_err(fd1, &normalHeader, sizeof(normalHeader));

    if (normalHeader.magic != 0x21796F4A) {
        errx(6, "Invalid magic number in data.bin");
    }

    ComparatorHeader comparatorHeader;
    read_err(fd2, &comparatorHeader, sizeof(comparatorHeader));

    if (comparatorHeader.magic1 != 0xAFBC7A37 || comparatorHeader.magic2 != 0x1C27) {
        errx(7, "Invalid magic numbers in comparator.bin");
    }

    for (uint64_t i = 0; i < comparatorHeader.count; i++) {
        ComparatorDataBlock data;
        read_err(fd2, &data, sizeof(data));

        uint64_t num1, num2;

        if (lseek(fd1, data.offset1 * sizeof(uint64_t), SEEK_SET) == -1) {
            err(4, "Failed to lseek");
        }
        read_err(fd1, &num1, sizeof(num1));

        if (lseek(fd1, data.offset2 * sizeof(uint64_t), SEEK_SET) == -1) {
            err(4, "Failed to lseek");
        }
        read_err(fd1, &num2, sizeof(num2));

        if (data.type == 0) {
            if (num1 > num2) {
                swapAndSave(fd1, num1, num2, data.offset1, data.offset2);
            }
        } else if (data.type == 1) {
            if (num1 < num2) {
                swapAndSave(fd1, num1, num2, data.offset1, data.offset2);
            }
        } else {
            err(5, "Data type should have values 0 or 1");
        }
    }

    close(fd1);
    close(fd2);
}