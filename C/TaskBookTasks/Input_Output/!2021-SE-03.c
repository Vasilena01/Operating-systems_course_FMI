#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
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

int asserted_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write");
    }
    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int fdInput = asserted_open(argv[1], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, &perms);

    struct stat st;
    if (fstat(fdInput, &st) == -1) {
        err(2, "Failed to fstat");
    }

    int elementsCount = st.st_size / 2;
    if (st.st_size % sizeof(uint16_t) != 0 || elementsCount > 524288) {
        err(2, "File is expected to have max 524288 elements in it");
    }

    char str1[] = "#include <stdint.h>\n\n";
    char str2[] = "uint32_t arrN = ";
    char str3[] = "const uint16_t arr[] = {";
    asserted_write(fdOutput, (void*)&str1, strlen(str1));
    asserted_write(fdOutput, (void*)&str2, strlen(str2));
    dprintf(fdOutput, "%d;\n", elementsCount);
    asserted_write(fdOutput, (void*)&str3, strlen(str3));

    uint16_t currElement;
    for (int i = 0; i < elementsCount - 1; i++) {
        asserted_read(fdInput, (void*)&currElement, sizeof(currElement));
        dprintf(fdOutput, "%d, ", currElement);
    }

    asserted_read(fdInput, (void*)&currElement, sizeof(currElement));
    dprintf(fdOutput, "%d}\n", currElement);

    close(fdInput);
    close(fdOutput);
}