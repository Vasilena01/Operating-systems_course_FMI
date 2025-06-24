// First solution
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

void asserted_unlink(const char* filename) {
    if (unlink(filename) < 0) {
        err(2, "Failed to unlink file");
    }
}

int asserted_open(const char* filename, int mode, int* perms, const char* tmpName) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, perms);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        asserted_unlink(tmpName);
        err(2, "Failed to open file");
    }

    return fd;
}

int asserted_read(int fd, void* buff, int size, const char* tmpName) {
    int r = read(fd, buff, size);

    if (r < 0) {
        asserted_unlink(tmpName);
        err(2, "Failed to read");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size, const char* tmpName) {
    int w = write(fd, buff, size);

    if (w < 0) {
        asserted_unlink(tmpName);
        err(2, "Failed to write");
    }

    return w;
}

int asserted_lseek(int fd, off_t offset, int whence, const char* tmpName) {
    int ls = lseek(fd, offset, whence);

    if (ls < 0) {
        asserted_unlink(tmpName);
        err(2, "Failed to lseek");
    }

    return ls;
}

typedef struct {
    uint64_t next; // index of the next node or 0
    uint8_t user_data[504];
} Data;

void syncFiles(int fdInput, int fdTmp, const char* tmpName) {
    asserted_lseek(fdTmp, 0, SEEK_SET, tmpName);

    uint8_t curr;
    while (asserted_read(fdTmp, &curr, sizeof(curr), tmpName) > 0) {
        asserted_write(fdInput, &curr, sizeof(curr), tmpName);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Expected 1 arg");
    }

    // We create a tmp file
    char tmpName[] = "/tmp/tmpNameXXXXXX";
    int fdTmp = mkstemp(tmpName);
    if (fdTmp < 0) {
        err(3, "Failed to open tmp file");
    }

    int fdInput = asserted_open(argv[1], O_RDWR, NULL, tmpName);

    struct stat st;
    if (fstat(fdInput, &st) < 0) {
        err(3, "Failed to fstat");
    }

    if (st.st_size % sizeof(Data) != 0) {
        errx(3, "Input file is not with correct size");
    }

    // Fill the tmp file with 0, to represent the original input file
    uint8_t emptyData[sizeof(Data)];
    for (long unsigned int i = 0; i < sizeof(Data); i++) {
        emptyData[i] = 0;
    }

    int allNodesCount = st.st_size / sizeof(Data);
    for (int i = 0; i < allNodesCount; i++) {
        asserted_write(fdTmp, &emptyData, sizeof(emptyData), tmpName);
    }

    // Read and rewrite valid nodes
    Data currData;
    // First read the first node
    asserted_read(fdInput, &currData, sizeof(currData), tmpName);
    asserted_write(fdTmp, &currData, sizeof(currData), tmpName);

    while (currData.next != 0) {
        // Read next node
        asserted_lseek(fdInput, currData.next * sizeof(Data), SEEK_SET, tmpName);
        asserted_read(fdInput, &currData, sizeof(currData), tmpName);
        // Write valid node to tmp file
        asserted_lseek(fdTmp, currData.next * sizeof(Data), SEEK_SET, tmpName);
        asserted_write(fdTmp, &currData, sizeof(currData), tmpName);
    }

    // Now we write all the info from the tmp file back to the original one
    int newInputFd = asserted_open(argv[1], O_WRONLY | O_TRUNC, NULL, tmpName);
    syncFiles(newInputFd, fdTmp, tmpName);

    asserted_unlink(tmpName);
    close(fdInput);
    close(fdTmp);
}

// Second solution
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint64_t next; // index of the next Node, from the beginning of the file or 0
    char user_data[504];
} Node;

int asserted_open(const char* filename, int mode, int* perms, const char* tmpName) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, &perms);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        unlink(tmpName);
        err(2, "Failed to open file");
    }

    return fd;
}

int asserted_read(int fd, void* buff, int size, const char* tmpName) {
    int r = read(fd, buff, size);

    if (r < 0) {
        unlink(tmpName);
        err(2, "Failed to read from file");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size, const char* tmpName) {
    int w = write(fd, buff, size);

    if (w < 0) {
        unlink(tmpName);
        err(2, "Failed to write to file");
    }

    return w;
}

int asserted_lseek(int fd, off_t offset, int whence, const char* tmpName) {
    int ls = lseek(fd, offset, whence);

    if (ls < 0) {
        unlink(tmpName);
        err(2, "Failed to lseek");
    }

    return ls;
}

void storeValidIndicies(int fd, int fdTmp, const char* tmpName) {
    int bytesRead;
    uint64_t currIndex = 0;
    asserted_write(fdTmp, &currIndex, sizeof(currIndex), tmpName);

    while ((bytesRead = asserted_read(fd, &currIndex, sizeof(currIndex), tmpName)) > 0) {
        if (currIndex == 0) {
            break;
        }

        asserted_write(fdTmp, &currIndex, sizeof(currIndex), tmpName);
        // We lseek to the next index of a node
        asserted_lseek(fd, currIndex * sizeof(Node), SEEK_SET, tmpName);
    }

    if (bytesRead < 0) {
        unlink(tmpName);
        err(4, "Failed to read bytes from file");
    }

    // Return pointers to the beginning of the files
    asserted_lseek(fd, 0, SEEK_SET, tmpName);
    asserted_lseek(fdTmp, 0, SEEK_SET, tmpName);
}

bool isValidIndex(uint64_t index, int fdTmp, const char* tmpName) {
    uint64_t currIndex;
    while (asserted_read(fdTmp, &currIndex, sizeof(currIndex), tmpName) > 0) {
        if (currIndex == index) {
            return true;
        }
    }

    return false;
}

void rewriteInvalidNodes(int fd, int fdTmp, const char* tmpName) {
    uint8_t zeros[512];
    memset(zeros, 0, sizeof(zeros));

    Node node;
    int bytesRead;
    uint64_t currIndex = 0;

    while ((bytesRead = asserted_read(fd, &node, sizeof(node), tmpName)) > 0) {
        if (!isValidIndex(currIndex, fdTmp, tmpName)) {
            // Go back 512 bytes back
            asserted_lseek(fd, -sizeof(node), SEEK_CUR, tmpName);

            // Write zero bytes to this node
            asserted_write(fd, &zeros, sizeof(zeros), tmpName);
        }

        currIndex++;
    }

    if (bytesRead < 0) {
        unlink(tmpName);
        err(4, "Failed to read bytes from file");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errx(1, "Expected 1 argument");
    }

    // Creating a temp file to store all valid indicies
    char tmpName[] = "/tmp/tmpFileXXXXXX";
    int fdTmp = mkstemp(tmpName);
    if (fdTmp < 0) {
        err(2, "Failed to create tmp file");
    }

    int fd = asserted_open(argv[1], O_RDWR, NULL, tmpName);
    struct stat st;

    if (fstat(fd, &st) < 0) {
        unlink(tmpName);
        err(2, "Failed to fstat");
    }

    if ((st.st_size % (512 * 8)) != 0) {
        unlink(tmpName);
        err(3, "File has invalid size");
    }

    // Store all valid indicies
    storeValidIndicies(fd, fdTmp, tmpName);

    // Rewrite invalid nodes
    rewriteInvalidNodes(fd, fdTmp, tmpName);

    unlink(tmpName);
    close(fdTmp);
    close(fd);
}