#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

int main(int argc, char* argv[]) {

    if (argc != 3 && argc != 5) {
        errx(1, "Expected 3 or 5 args");
    }

    char buff[1024];
    int readRes = read(0, &buff, sizeof(char));
    if (readRes != sizeof(char)) {
        errx(2, "Failed to read from standard input");
    }

    if (strcmp(argv[1], "-c") == 0) {

        if (strlen(argv[2]) == 1 && argv[2][0] >= '1' && argv[2][0] <= '9') {
            int index = atoi(argv[2]);
            int writeRes = write(1, &buff[index], sizeof(buff[index]));
            if (writeRes != sizeof(buff[index])) {
                err(3, "Failed to write to standard output");
            }
            putchar(buff[index]);
        } else if (strlen(argv[2]) == 3 && argv[2][0] >= '1' && argv[2][0] <= '9'
                   && argv[2][1] == '-' && argv[2][2] >= '1' && argv[2][2] <= '9') {
            int lower, upper;
            char* dash = strchr(argv[4], '-');
            if (dash == NULL) {
                lower = upper = atoi(argv[4]);
            } else {
                *dash = '\0';
                lower = atoi(argv[4]);
                upper = atoi(dash + 1);
                if (lower > upper) {
                    errx(4, "Lower bound > upper bound");
                }
            }

            if (lower > upper) {
                err(4, "Lower bound should be smaller than upper bound");
            }
            putchar(buff[lower]);
            putchar(buff[upper]);

            for (int i = lower; i <= upper; i++) {
                int writeRes = write(1, &buff[i], sizeof(buff[i]));
                if (writeRes != sizeof(buff[i])) {
                    err(5, "Failed to write to standard output");
                }
            }
        } else {
            err(6, "Invalid args");
        }
    } else if (strcmp(argv[1], "-d") == 0) {

        if (strlen(argv[2]) != 3 || argv[2][0] != '\'' || argv[2][2] != '\'') {
            errx(6, "Invalid delimiter format");
        }
        char delim = argv[2][1];

        if (strcmp(argv[3], "-f") != 0) {
            errx(6, "Expected -f after delimiter");
        }

        int lower, upper;
        char* dash = strchr(argv[4], '-');
        if (dash == NULL) {
            lower = upper = atoi(argv[4]);
        } else {
            *dash = '\0';
            lower = atoi(argv[4]);
            upper = atoi(dash + 1);
            if (lower > upper) {
                errx(4, "Lower bound > upper bound");
            }
        }

        if (lower < 1) {
            errx(6, "Field range must start from 1");
        }

        int field = 1;
        char* p = buff;
        char* end = buff + readRes;

        while (p < end) {
            char* next = p;
            while (next < end && *next != delim && *next != '\n') {
                next++;
            }

            if (field >= lower && field <= upper) {
                write(1, p, next - p);
                if (field != upper) {
                    write(1, &delim, 1);
                }
            }

            if (next < end && (*next == delim || *next == '\n')) {
                next++;
            }

            p = next;
            field++;

            if (field > upper) break;
        }
    }
}
