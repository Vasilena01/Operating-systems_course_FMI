#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        errx(1, "Args count must be 3");
    }

    const char* f1 = argv[1];
    const char* f2 = argv[2];
    const char* patch = argv[3];

    int fd1 = open(f1, O_RDONLY);
    if (fd1 == -1) {
        err(2, "Couldn't open file %s", f1);
    }

    int fd2 = open(f2, O_RDONLY);
    if (fd2 == -1) {
        err(2, "Couldn't open file %s", f2);
    }

    int fdPatch = open(patch, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fdPatch == -1) {
        err(2, "Couldn't open file %s", patch);
    }

    struct stat st1;
    struct stat st2;
    if (fstat(fd1, &st1) == -1 || fstat(fd2, &st2) == -1) {
        errx(3, "Couldn't stat either f1 or f2");
    }

    if (st1.st_size != st2.st_size) {
        err(4, "File1 and file2 should have equal sizes");
    }

    uint16_t position = 0;
    uint8_t charf1;
    uint8_t charf2;
    int bytesCount1;
    int bytesCount2;

    while ((bytesCount1 = read(fd1, &charf1, sizeof(charf1))) == sizeof(charf1) &&
           (bytesCount2 = read(fd2, &charf2, sizeof(charf2))) == sizeof(charf2)) {
        position++;

        if (charf1 != charf2) {
            int wPos = write(fdPatch, &position, sizeof(position));
            if (wPos != sizeof(position)) {
                err(5, "Couldn't write position to file %s", patch);
            }

            int wChar1 = write(fdPatch, &charf1, sizeof(charf1));
            if (wChar1 != sizeof(charf1)) {
                err(5, "Couldn't write char1 to file %s", f1);
            }

            int wChar2 = write(fdPatch, &charf2, sizeof(charf2));
            if (wChar2 != sizeof(charf2)) {
                err(5, "Couldn't write char2 to file %s", f2);
            }
        }
    }

    if (bytesCount1 == -1) {
        err(5, "Couldn't read bytes from file %s", f1);
    }

    if (bytesCount2 == -1) {
        err(5, "Couldn't read bytes from file %s", f2);
    }

    close(fd1);
    close(fd2);
    close(fdPatch);

    return 0;
}
