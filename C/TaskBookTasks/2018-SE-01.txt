#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <stdbool.h>

bool isCharInSet(const char* set, char ch) {
    size_t len = strlen(set);
    for (size_t i = 0; i < len; i++) {
        if (set[i] == ch) {
            return true;
        }
    }
    return false;
}

int getCharIndex(const char* set, char ch) {
    size_t len = strlen(set);
    for (size_t i = 0; i < len; i++) {
        if (set[i] == ch) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        errx(1, "Expected at least 2 arguments");
    }

    const char* option = argv[1];

    if (strcmp(option, "-d") == 0) {
        if (argc != 3) {
            errx(1, "Expected exactly 2 arguments: -d and SET1");
        }

        const char* set = argv[2];
        if (strlen(set) == 0) {
            errx(1, "SET1 cannot be empty");
        }

        char ch;
        ssize_t bytesCount;
        while ((bytesCount = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            if (isCharInSet(set, ch)) {
                continue;
            }

            if (write(1, &ch, bytesCount) != bytesCount) {
                err(2, "Couldn't write character to standard output");
            }
        }

        if (bytesCount < 0) {
            err(2, "Couldn't read character from standard input");
        }

    } else if (strcmp(option, "-s") == 0) {
        if (argc != 3) {
            errx(1, "Expected exactly 2 arguments: -s and SET1");
        }

        const char* newChar = argv[2];
        if (strlen(newChar) != 1) {
            errx(1, "SET1 in -s mode must be exactly 1 character");
        }

        char ch;
        ssize_t bytesCount;
        bool isCharMet = false;
        while ((bytesCount = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            if (ch == newChar[0]) {
                isCharMet = true;
                continue;
            }

            if (isCharMet) {
                if (write(1, newChar, 1) != 1) {
                    err(2, "Couldn't write squeezed character");
                }
                isCharMet = false;
            }

            if (write(1, &ch, bytesCount) != bytesCount) {
                err(2, "Couldn't write normal character");
            }
        }

        if (bytesCount < 0) {
            err(2, "Couldn't read character from standard input");
        }

    } else {
        if (argc != 3) {
            errx(1, "Expected exactly 2 arguments: SET1 and SET2");
        }

        const char* set1 = argv[1];
        const char* set2 = argv[2];

        size_t len1 = strlen(set1);
        size_t len2 = strlen(set2);

        if (len1 != len2) {
            errx(3, "SET1 and SET2 must have equal lengths");
        }

        if (len1 == 0) {
            errx(1, "SET1 and SET2 cannot be empty");
        }

        char ch;
        ssize_t bytesCount;
        while ((bytesCount = read(0, &ch, sizeof(ch))) == sizeof(ch)) {
            char newChar = ch;
            int index = getCharIndex(set1, ch);
            if (index != -1) {
                newChar = set2[index];
            }

            if (write(1, &newChar, bytesCount) != bytesCount) {
                err(2, "Couldn't write character to standard output");
            }
        }

        if (bytesCount < 0) {
            err(2, "Couldn't read character from standard input");
        }
    }

    return 0;
}