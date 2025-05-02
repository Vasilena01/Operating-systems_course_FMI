#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
    uint32_t number;
    uint32_t times;
} interval;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "Expected 3 args");
    }

    const char* f1 = argv[1];
    const char* f2 = argv[2];
    const char* f3 = argv[3];

    int fd1 = open(f1, O_RDONLY);
    if (fd1 == -1) {
        err(2, "Couldn't open file %s", f1);
    }

    int fd2 = open(f2, O_RDONLY);
    if (fd2 == -1) {
        err(2, "Couldn't open file %s", f2);
    }

    int fd3 = open(f3, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd3 == -1) {
        err(2, "Couldn't open file %s", f3);
    }

    struct stat st;
    if (fstat(fd1, &st) == -1) {
       err(3, "Fstat failed");
    }

    if (st.st_size % (2 * sizeof(uint32_t)) != 0) {
       errx(4, "f1 size doesn't have correct size (/8)");
    }

    interval curr;
    int bytesCount;

    while ((bytesCount = read(fd1, &curr, sizeof(curr))) == sizeof(curr)) {
        if (lseek(fd2, curr.number * sizeof(uint32_t), SEEK_SET) == -1) {
            err(5, "Couldn't seek to position %u", curr.number);
        }

        uint32_t currNum;
        for (uint32_t i = 0; i < curr.times; i++) {
            if (read(fd2, &currNum, sizeof(currNum)) != sizeof(currNum)) {
                errx(6, "Couldn't read bytes from file %s", f2);
            }

            if (write(fd3, &currNum, sizeof(currNum)) != sizeof(currNum)) {
                errx(6, "Couldn't write bytes to file %s", f3);
            }
        }
    }

    if (bytesCount == -1) {
        errx(4, "Couldn't read from file %s", f1);
    }

    close(fd1);
    close(fd2);
    close(fd3);
}