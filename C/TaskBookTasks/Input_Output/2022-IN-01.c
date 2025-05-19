#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

typedef struct {
    uint16_t magic; //0x5A4D
    uint16_t fileType; // 1 - list.bin, 2 - data.bin, 3 - out.bin
    uint32_t count;
} Header;

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
        err(2, "Failed to read file");
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

int asserted_lseek(int fd, off_t offset, int whence) {
    int ls = lseek(fd, offset, whence);

    if (ls < 0) {
        err(2, "Failed to lseek file");
    }

    return ls;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        err(1, "Expected 3 args");
    }

    int fdList = asserted_open(argv[1], O_RDONLY, NULL);
    int fdInput = asserted_open(argv[2], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdOutput = asserted_open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    struct stat stList, stInput;
    if (fstat(fdList, &stList) < 0 ||
        fstat(fdInput, &stInput) < 0) {
        err(3, "Failed to fstat");
    }

    Header listHeader, inputHeader, outputHeader;
    asserted_read(fdList, &listHeader, sizeof(listHeader));
    asserted_read(fdInput, &inputHeader, sizeof(inputHeader));

    if (listHeader.magic != 0x5A4D || inputHeader.magic != 0x5A4D) {
        err(3, "Invalid magic num");
    }

    if (listHeader.fileType != 1 || inputHeader.fileType != 2) {
        err(3, "Invalid filetype");
    }

    if (stList.st_size % listHeader.count != 0 ||
        stInput.st_size % inputHeader.count != 0) {
        err(3, "Incorrect file size");
    }

    // Read header info for output file
    uint16_t maxCount = 0;
    uint16_t currNum;
    for (uint16_t i = 0; i < listHeader.count; i++) {
        asserted_read(fdList, &currNum, sizeof(currNum));
        if (currNum > maxCount) {
            maxCount = currNum;
        }
    }

    // Create and write new header to output file
    outputHeader.count = maxCount;
    outputHeader.magic = 0x5A4D;
    outputHeader.fileType = 3;
    asserted_write(fdOutput, &outputHeader, sizeof(outputHeader));

    uint16_t dummy = 0;
    // Write dummy value = 0 for whole output file
    for (size_t i = 0; i < maxCount; i++) {
        asserted_write(fdOutput, &dummy, sizeof(dummy));
    }

    // Read byte by byte from list file
    uint16_t curr;
    int headerLen = sizeof(Header);

    for (uint32_t i = 0; i < listHeader.count; i++) {
        asserted_lseek(fdList, headerLen + i * sizeof(uint16_t), SEEK_SET);
        asserted_read(fdList, &curr, sizeof(curr));

        // Lseek input by i and output by curr
        asserted_lseek(fdInput, headerLen + i * sizeof(uint32_t), SEEK_SET);

        uint32_t numInInput;
        asserted_read(fdInput, &numInInput, sizeof(numInInput));
        if (numInInput == 0) {
            continue;
        }

        asserted_lseek(fdOutput, curr * sizeof(uint64_t), SEEK_SET);
        asserted_write(fdOutput, &numInInput, sizeof(numInInput));
    }

    close(fdList);
    close(fdInput);
    close(fdOutput);
}