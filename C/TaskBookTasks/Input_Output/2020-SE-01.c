#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

typedef struct {
    uint32_t reserved1;
    uint16_t len;
    uint16_t reserved2;
    uint64_t reserved3;
} Header;

typedef struct {
    uint16_t postfixPosition;
    uint16_t postfixElementsCount;

    uint16_t prefixPosition;
    uint16_t prefixElementsCount;

    uint16_t infixPosition;
    uint16_t infixElementsCount;

    uint16_t suffixPosition;
    uint16_t suffixElementsCount;
} Complect;

int elSize[4] = { 4, 1, 2, 8 };

int asserted_open(const char* filename, int mode, int* options) {
    int fd;

    if (options != NULL) {
        fd = open(filename, mode, *options);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open %s", filename);
    }

    return fd;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Failed to read from %d", fd);
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write to %d", fd);
    }
    return w;
}

off_t asserted_lseek(int fd, off_t offset, int whence) {
    off_t l = lseek(fd, offset, whence);

    if (l < 0) {
        err(3, "Failed to lseek %d", fd);
    }

    return l;
}

int asserted_fstat(int fd, struct stat* st) {
    int f;
    if ((f = fstat(fd, *st)) == -1) {
        err(3, "Failed to fstat");
    }

    return f;
}

void validateFile(int fd, int i) {
    Header h;
    struct stat st;
    ssize_t fileSize;

    asserted_read(fd, (void*)&h, sizeof(h));
    asserted_fstat(fd, &st);
    fileSize = st.st_size;

    if ((fileSize - 16) % elSize[i] != 0) {
        err(3, "Invalid file structure");
    }

    if ((fileSize - 16) / elSize[i] != h.len) {
        err(3, "Invalid element count in file %d", fd);
    }
}

int main(int argc, char* argv[]) {

    if (argc != 7) {
        err(1, "Expected 6 files to be passed as args");
    }

    int fdAffix = asserted_open(argv[1], O_RDONLY, NULL);
    int fdPostfix = asserted_open(argv[2], O_RDONLY, NULL);
    int fdPrefix = asserted_open(argv[3], O_RDONLY, NULL);
    int fdInfix = asserted_open(argv[4], O_RDONLY, NULL);
    int fdSuffix = asserted_open(argv[5], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdCrucifixus = asserted_open(argv[6], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    validateFile(fdAffix, 2);
    validateFile(fdPostfix, 0);
    validateFile(fdPrefix, 1);
    validateFile(fdInfix, 2);
    validateFile(fdSuffix, 3);

    Complect complect;
    while (asserted_read(fdAffix, (void*)&complect, sizeof(complect)) > 0) {

        asserted_lseek(fdPostfix, complect.postfixPosition * sizeof(uint32_t), SEEK_SET);
        for (int i = 0; i < complect.postfixElementsCount; i++) {
            uint32_t el;
            asserted_read(fdPostfix, (void*)&el, sizeof(el));
            asserted_write(fdCrucifixus, (void*)&el, sizeof(el));
        }

        asserted_lseek(fdPrefix, complect.prefixPosition * sizeof(uint8_t), SEEK_SET);
        for (int i = 0; i < complect.prefixElementsCount; i++) {
            uint8_t el;
            asserted_read(fdPrefix, (void*)&el, sizeof(el));
            asserted_write(fdCrucifixus, (void*)&el, sizeof(el));
        }

        asserted_lseek(fdInfix, complect.infixPosition * sizeof(uint16_t), SEEK_SET);
        for (int i = 0; i < complect.infixElementsCount; i++) {
            uint16_t el;
            asserted_read(fdInfix, (void*)&el, sizeof(el));
            asserted_write(fdCrucifixus, (void*)&el, sizeof(el));
        }

        asserted_lseek(fdSuffix, complect.suffixPosition * sizeof(uint64_t), SEEK_SET);
        for (int i = 0; i < complect.suffixElementsCount; i++) {
            uint64_t el;
            asserted_read(fdSuffix, (void*)&el, sizeof(el));
            asserted_write(fdCrucifixus, (void*)&el, sizeof(el));
        }
    }

    close(fdAffix);
    close(fdPostfix);
    close(fdPrefix);
    close(fdInfix);
    close(fdSuffix);
    close(fdCrucifixus);
}