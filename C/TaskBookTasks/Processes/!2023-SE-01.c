#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <string.h>

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

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write to file");
    }

    return w;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Failed to read from file");
    }

    return r;
}

void getFilenameHash(char* filename, char* hash) {
    int pd[2];
    if (pipe(pd) < 0) {
        err(5, "Failed to pipe");
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        err(5, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd[0]);

        if (dup2(pd[1], 1) < 0) {
            err(5, "Failed to dup2");
        }

        if (execlp("md5sum", "md5sum", filename, (void*)NULL) < 0) {
            err(6, "Failed to execute command md5sum");
        }
    }

    close(pd[1]);
    if (dup2(pd[0], 0) < 0) {
        err(5, "Failed to dup2");
    }

    int index = 0;
    char ch;
    while (asserted_read(pd[0], &ch, sizeof(ch)) > 0) {
        if (ch == ' ') {
            break;
        }
        hash[index++] = ch;
    }

    hash[index] = '\0';
    wait(NULL); // avoid zombies
}

void createHashFile(char* filename) {
    // Check if filename ends with .hash
    size_t len = strlen(filename);
    if (len >= 5) {
        size_t start = len - 5;
        if (filename[start] == '.') {
            char extension[6];
            int index = 0;
            for (size_t i = start + 1; i < len; i++) {
                extension[index++] = filename[i];
            }

            extension[index] = '\0';

            if (strcmp(extension, "hash") == 0) {
                return;
            }
        }
    }

    // Get filename hash
    char hash[256];
    getFilenameHash(filename, hash);

    // Add .hash at the end of the filename
    filename[len] = '.';
    filename[len + 1] = 'h';
    filename[len + 2] = 'a';
    filename[len + 3] = 's';
    filename[len + 4] = 'h';
    filename[len + 5] = '\0';

    // Write hash to file
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, &perms);
    asserted_write(fd, hash, strlen(hash));
    close(fd);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        errx(1, "Expected 1 arg");
    }

    // find <dirname> -type f
    int pd[2];
    if (pipe(pd) < 0) {
        err(3, "Failed to pipe");
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        err(3, "Failed to fork");
    }

    if (child_pid == 0) {
        close(pd[0]);

        if (dup2(pd[1], 1) < 0) {
            err(3, "Failed to dup2");
        }

        if (execlp("find", "find", argv[1], "-type", "f", (void*)NULL) < 0) {
            err(4, "Failed to execute find command");
        }
    }

    close(pd[1]);

    if (dup2(pd[0], 0) < 0) {
        err(3, "Failed to dup2");
    }

    // Read file by file from dir and its subdirs
    char ch;
    int index = 0;
    char filename[256];
    while (asserted_read(pd[0], &ch, sizeof(ch)) > 0) {
        if (ch == '\n') {
            filename[index] = '\0';
            index = 0;
            createHashFile(filename);
            continue;
        }

        filename[index++] = ch;
    }

    wait(NULL);
}