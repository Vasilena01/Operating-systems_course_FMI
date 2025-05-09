#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <err.h>

typedef struct {
    uint32_t magic;
    uint8_t headerVersion;
    uint8_t dataVersion;
    uint16_t count;
    uint32_t reserved1;
    uint32_t reserved2;
} Header;

typedef struct {
    uint16_t offset;
    uint8_t originalByte;
    uint8_t newByte;
} Data1; // by version 0x00

typedef struct {
    uint32_t offset;
    uint16_t originalWord;
    uint16_t newWord;
} Data2; // by version 0x01

int asserted_open(const char* filename, int mode, int* options) {
    int fd;

    if (options != NULL) {
        fd = open(filename, mode, *options);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }

    return fd;
}

off_t asserted_lseek(int fd, off_t offset, int whence) {
    off_t ls = lseek(fd, offset, whence);

    if (ls < 0) {
        err(2, "Failed to lseek %d", fd);
    }

    return ls;
}

int asserted_read(int fd, void* buff, int size) {
    int bytesCount;

    if ((bytesCount = read(fd, buff, size)) < 0) {
        err(2, "Failed to read from %d", fd);
    }

    return bytesCount;
}

int asserted_write(int fd, void* buff, int size) {
    int bytesCount;

    if ((bytesCount = write(fd, buff, size)) < 0) {
        err(2, "Failed to write to %d", fd);
    }

    return bytesCount;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        err(1, "Expected 3 args: ./main patch.bin f1.bin f2.bin");
    }

    int fdPatch = asserted_open(argv[1], O_RDONLY, NULL);
    int fdFile1 = asserted_open(argv[2], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdFile2 = asserted_open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    Header header;
    asserted_read(fdPatch, (void*)&header, sizeof(header));   // <-- fixed: read patch header from patch.bin

    if (header.magic != 0xEFBEADDE || header.headerVersion != 0x01) {
        err(3, "Patch file doesn't follow the needed specification or header version mismatch");
    }

    if (header.dataVersion == 0x00) {
        Data1 data;
        uint16_t lastAccessedIndex = 0;
        uint8_t currByte;

        while (asserted_read(fdPatch, (void*)&data, sizeof(data)) > 0) {

            // Write all bytes before offset
            for (uint16_t i = lastAccessedIndex; i < data.offset; i++) {
                asserted_read(fdFile1, (void*)&currByte, sizeof(currByte));
                asserted_write(fdFile2, (void*)&currByte, sizeof(currByte));
            }

            // Read byte at position offset
            asserted_read(fdFile1, (void*)&currByte, sizeof(currByte));
            if (currByte == data.originalByte) {
                asserted_write(fdFile2, (void*)&data.newByte, sizeof(data.newByte));
            }
            else {
                err(7, "Current byte is not equal to original byte at offset %u", data.offset);
            }

            lastAccessedIndex = data.offset + 1;
        }

        // Write the rest of the data from file1 to file2
        while (asserted_read(fdFile1, (void*)&currByte, sizeof(currByte))) {
            asserted_write(fdFile2, (void*)&currByte, sizeof(currByte));
        }

    }
    else if (header.dataVersion == 0x01) {
        Data2 data;
        uint32_t lastAccessedIndex = 0;
        uint16_t currWord;

        while (asserted_read(fdPatch, (void*)&data, sizeof(data)) > 0) {

            // Write all words before offset
            for (uint32_t i = lastAccessedIndex; i < data.offset; i++) {
                asserted_read(fdFile1, (void*)&currWord, sizeof(currWord));
                asserted_write(fdFile2, (void*)&currWord, sizeof(currWord));
            }

            // Read word at position offset
            asserted_read(fdFile1, (void*)&currWord, sizeof(currWord));
            if (currWord == data.originalWord) {
                asserted_write(fdFile2, (void*)&data.newWord, sizeof(data.newWord));
            }
            else {
                err(7, "Current word is not equal to original word at offset %u", data.offset);
            }

            lastAccessedIndex = data.offset + 1;
        }

        // Write the rest of the data from file1 to file2
        while (asserted_read(fdFile1, (void*)&currWord, sizeof(currWord))) {
            asserted_write(fdFile2, (void*)&currWord, sizeof(currWord));
        }

    }
    else {
        err(3, "Can't read data from unknown data version %02x", header.dataVersion);
    }

    // Close all file descriptors
    close(fdPatch);
    close(fdFile1);
    close(fdFile2);

    return 0;
}