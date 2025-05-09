#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

int main(int argc, char* argv[]) {

    if (argc == 1) {
        err(1, "Expected min 1 arg");
    }

    for (int i = 1; i < argc; i++) {
        int fd;
        if (strcmp(argv[i], "-") == 0) {
            fd = 0;
        } else {
            fd = open(argv[i], O_RDONLY);
            if (fd == -1) {
                err(2, "Failed to open file for read");
            }
        }

        if (fd == 0) {
            char ch;
            while (read(fd, &ch, sizeof(ch)) == sizeof(ch)) {
                if (write(1, &ch, sizeof(ch)) != 1) {
                    err(2, "Failed to write to standart output");
                }

                if (ch == '\n') {
                    break; // stop reading after newline from standart input
                }
            }
        } else {
            char buff[4096];
            int bytesCount;
            while ((bytesCount = read(fd, &buff, sizeof(buff))) > 0) {
                if (write(1, &buff, bytesCount) != bytesCount) {
                    err(2, "Failed to write to standart output");
                }
            }

            if (bytesCount == -1) {
                err(3, "Failed to read from file / standart input");
            }

            close(fd);
        }
    }
}