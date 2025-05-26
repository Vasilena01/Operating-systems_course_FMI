#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;
    if (perms != NULL) {
        fd = open(filename, mode, &perms);
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
        err(3, "Failed to read from file");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write to file");
    }

    return w;
}

int asserted_lseek(int fd, off_t offset, int whence) {
    int ls = lseek(fd, offset, whence);

    if (ls < 0) {
        errx(3, "Failed to lseek");
    }

    return ls;
}

void binarySearchWord(int fd, const char* target) {
    off_t low = 0;
    off_t high = asserted_lseek(fd, 0, SEEK_END);

    while (low < high) {

        off_t mid = low + (high - low) / 2;

        // Seeking to the middle of the file
        asserted_lseek(fd, mid, SEEK_SET);

        char currByte;
        int bytesRead;
        while ((bytesRead = asserted_read(fd, &currByte, sizeof(currByte))) > 0) {
            if (currByte == '\0') {
                break;
            }
        }

        if (bytesRead < 0) {
            err(4, "Failed to read bytes from file");
        }

        // We have probably found the nearest word and now we read it
        char word[64];
        char ch;
        int wordLen = 0;
        while (asserted_read(fd, &ch, sizeof(ch)) > 0) {
            if (ch == '\n' || ch == '\0') {
                break;
            }

            word[wordLen++] = ch;
        }

        word[wordLen] = '\0';

        // We compare both words
        if (strcmp(target, word) == 0) {
            int read;
            char desc;
            while ((read = asserted_read(fd, &desc, sizeof(desc))) > 0) {
                if (desc == '\0') {
                    break;
                }
                asserted_write(1, &desc, sizeof(desc));
            }

            if (read < 0) {
                err(6, "Failed to read description from file");
            }
            return;
        }
        else if (strcmp(target, word) < 0) {
            high = mid;
        }
        else if (strcmp(target, word) > 0) {
            low = mid + 1;
        }
    }

    // If no match
    errx(7, "No word like the target one was found");
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    const char* target = argv[1];
    if (strlen(target) > 63) {
        err(4, "Target word is expected to have len between 1 - 63");
    }

    int fd = asserted_open(argv[2], O_RDONLY, NULL);

    binarySearchWord(fd, target);

    close(fd);
    exit(0);
}
