#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        errx(1, "Expected more than 1 arg: option file");
    }

    if (strcmp(argv[1], "-n") == 0) {
        int line_count = 1;
        for (int j = 2; j < argc; j++) {
            const char* curr = argv[j];
            int fdCurr;
            if (*curr == '-') {
                fdCurr = 0;
            } else {
                fdCurr = open(curr, O_RDONLY);
                if (fdCurr == -1) {
                    err(2, "Couldn't read from file %s", curr);
                }
            }

            char buff[1024];
            int bytesCount = 0;

            while ((bytesCount = read(fdCurr, &buff, sizeof(buff))) > 0) {
                for (int i = 0; i < bytesCount; i++) {
                    if (i == 0 || buff[i - 1] == '\n') {
                        char lines_str[32];
                        int len = snprintf(lines_str, sizeof(lines_str), "%d ", line_count++);
                        if (write(1, lines_str, len) != len) {
                            err(2, "Couldn't write line number");
                        }
                    }

                    if (write(1, &buff[i], 1) != 1) {
                        err(2, "Couldn't write from file to standart output");
                    }
                }
            }

            if (bytesCount == -1) {
                err(2, "Couldn't read any bytes from file %s", curr);
            }
        }
    } else {
        for (int i = 1; i < argc; i++) {
            const char* curr = argv[i];
            int fdCurr;
            if (*curr == '-') {
                fdCurr = 0;
            } else {
                fdCurr = open(curr, O_RDONLY);
                if (fdCurr == -1) {
                    err(2, "Couldn't read from file %s", curr);
                }
            }

            char buff[1024];
            int bytesCount = 0;
            while ((bytesCount = read(fdCurr, &buff, sizeof(buff))) > 0) {
                if (write(1, &buff, bytesCount) != bytesCount) {
                    err(2, "Couldn't write from file to standart output");
                }
            }

            if (bytesCount == -1) {
                err(2, "Couldn't read any bytes from file %s", curr);
            }
        }
    }
}