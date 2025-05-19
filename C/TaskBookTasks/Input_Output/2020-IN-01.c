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

int asserted_lseek(int fd, off_t offset, int whence) {
    int ls = lseek(fd, offset, whence);

    if (ls < 0) {
        err(2, "Failed to lseek");
    }

    return ls;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        err(1, "Expected 3 args");
    }

    int fdPatch = asserted_open(argv[1], O_RDONLY, NULL);
    int fdFile1 = asserted_open(argv[2], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdFile2 = asserted_open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, &perms);

    Header header;
    asserted_read(fdPatch, (void*)&header, sizeof(header));

    if (header.magic != 0xEFBEADDE || header.headerVersion != 0x01) {
        err(3, "Patch file doesn't follow the needed specification or header version is wrong");
    }

    // Check if patch file has correct size
    struct stat st;
    if (fstat(fdPatch, &st) == -1) {
        err(3, "Failed to fstat");
    }

    if (header.dataVersion == 0x00) {
        Data1 data;

        if (st.st_size != (long int)(sizeof(data) * header.count)) {
            err(4, "Incorrect patch file size");
        }

        uint8_t currByte;
        int bytesRead;
        while ((bytesRead = asserted_read(fdFile1, &currByte, sizeof(currByte))) > 0) {
            asserted_write(fdFile2, &currByte, sizeof(currByte));
        }

        if (bytesRead < 0) {
            err(4, "Failed to read from file1");
        }

        while (asserted_read(fdPatch, &data, sizeof(data)) > 0) {
            asserted_lseek(fdFile2, data.offset, SEEK_SET);

            // Read and write byte at position offset
            asserted_read(fdFile2, &currByte, sizeof(currByte));
            if (currByte == data.originalByte) {
                asserted_lseek(fdFile2, data.offset, SEEK_SET);
                asserted_write(fdFile2, &data.newByte, sizeof(data.newByte));
            }
            else {
                err(7, "Current byte is not equal to original byte");
            }
        }
    }
    else if (header.dataVersion == 0x01) {
        Data2 data;
        if (st.st_size != (long int)(sizeof(data) * header.count)) {
            err(4, "Incorrect patch file size");
        }

        uint16_t currWord;
        int bytesRead;
        while ((bytesRead = asserted_read(fdFile1, &currWord, sizeof(currWord))) > 0) {
            asserted_write(fdFile2, &currWord, sizeof(currWord));
        }

        if (bytesRead < 0) {
            err(4, "Failed to read from file1");
        }

        while (asserted_read(fdPatch, &data, sizeof(data)) > 0) {
            asserted_lseek(fdFile2, data.offset, SEEK_SET);
            // Read and write word at position offset
            asserted_read(fdFile2, &currWord, sizeof(currWord));
            if (currWord == data.originalWord) {
                asserted_lseek(fdFile2, data.offset, SEEK_SET);
                asserted_write(fdFile2, &data.newWord, sizeof(data.newWord));
            }
            else {
                err(7, "Current word is not equal to original word");
            }
        }
    }
    else {
        err(3, "Can't read data from unknown version");
    }

    close(fdPatch);
    close(fdFile1);
    close(fdFile2);
}