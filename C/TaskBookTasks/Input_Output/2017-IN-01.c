#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

typedef struct {
    uint16_t offset;
    uint8_t length;
    uint8_t padding;
} record;

int main(int argc, char* argv[]) {

    if (argc != 5) {
        errx(1, "Expected 4 args");
    }

    int fdDat1 = open(argv[1], O_RDONLY);
    if (fdDat1 == -1) {
        err(2, "Couldn't open file %s", argv[1]);
    }

    int fdIndx1 = open(argv[2], O_RDONLY);
    if (fdIndx1 == -1) {
        err(2, "Couldn't open file %s", argv[2]);
    }

    int fdDat2 = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fdDat2 == -1) {
        err(2, "Couldn't open file %s", argv[3]);
    }

    int fdIndx2 = open(argv[4], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fdIndx2 == -1) {
        err(2, "Couldn't open file %s", argv[4]);
    }

    struct stat st;
    if (fstat(fdIndx1, &st) == -1) {
        err(3, "failed to fstat f1Indx");
    }

    if (st.st_size % sizeof(record) != 0) {
        err(4, "f1.idx has invalid size");
    }

    record rec;
    record newrec;
    int entries = st.st_size / sizeof(rec);
    uint16_t offset = 0;
    uint8_t word[256];

    for (int i = 0; i < entries; i++) {

        if (read(fdIndx1, &rec, sizeof(rec)) != sizeof(rec)) {
            err(5, "Couldn't read fdIndx1");
        }

        if (lseek(fdDat1, rec.offset, SEEK_SET) == -1) {
            err(5, "Couldn't seek fdDat1");
        }

        if (rec.length == 0 || rec.length > sizeof(word)) {
            continue;
        }

        if (read(fdDat1, &word, rec.length) != rec.length) {
            err(5, "Couldn't read word from file %s", argv[1]);
        }

        if (word[0] >= 'A' && word[0] <= 'Z') {
            int wfdDat2 = write(fdDat2, &word, rec.length);
            if (wfdDat2 != rec.length) {
                err(5, "Couldn't write word to f2Dat");
            }

            newrec.offset = offset;
            newrec.length = rec.length;
            newrec.padding = 0;

            int wfdIndx2 = write(fdIndx2, &newrec, sizeof(newrec));
            if (wfdIndx2 != sizeof(newrec)) {
                err(5, "Couldn't write rec to f2Indx");
            }

            offset += rec.length;
        }
    }

    close(fdDat1);
    close(fdIndx1);
    close(fdDat2);
    close(fdIndx2);

    return 0;
}