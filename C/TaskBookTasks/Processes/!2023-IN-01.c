#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/wait.h>

int NC;
int WC;
int pds[8][2];
char L[3][4] = { "tic ", "tac ", "toe\n" };

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write to fd");
    }

    return w;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(3, "Failed to read from fd");
    }

    return r;
}

void close_pipes(int index) {
    for (int i = 0; i < NC + 1; i++) {
        if (i == index) {
            close(pds[index][0]);
        }
        else if (i == index - 1 && index > 0) {
            close(pds[index - 1][1]);
        }
        else if (i == NC && index == 0) {
            close(pds[NC][1]);
        }
        else {
            close(pds[i][0]);
            close(pds[i][1]);
        }
    }
}

void output_word(int index) {
    int word_index;
    int fd;
    if (index == 0) {
        fd = pds[NC][0];
    }
    else {
        fd = pds[index - 1][0];
    }

    while (asserted_read(fd, &word_index, sizeof(word_index)) > 0) {
        // If there're no words left to output, just return
        if (word_index >= WC) {
            asserted_write(pds[index][1], &word_index, sizeof(word_index));
            if (index == 0) {
                close(pds[index][1]);
                close(pds[NC][0]);
            }
            else {
                close(pds[index][1]);
                close(pds[index - 1][0]);
            }
            break;
        }

        asserted_write(1, L[word_index % 3], sizeof(L[word_index % 3]));
        word_index++;
        asserted_write(pds[index][1], &word_index, sizeof(word_index));
    }
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int index = 0;
    int word_index = 0;
    NC = atoi(argv[1]);
    WC = atoi(argv[2]);

    if (NC < 1 || NC > 7) {
        err(2, "Invalid value of NC");
    }

    if (WC < 1 || WC > 35) {
        err(2, "Invalid value of WC");
    }

    // We create NC + 1 pipes, in order the child processes + the parent to comunicate
    for (int i = 0; i < NC + 1; i++) {
        // We create a pipe for each process
        if (pipe(pds[i]) < 0) {
            err(4, "Failed to create pipe");
        }
    }

    // We create all children and output their corresponding word
    pid_t child_pid;
    for (int i = 1; i <= NC; i++) {
        if ((child_pid = fork()) == -1) {
            err(5, "Failed to fork child");
        }

        if (child_pid == 0) {
            close_pipes(i);
            output_word(i);
            exit(0);
        }
    }

    // Parent section
    close_pipes(0);

    // If we are at the first parent process - we don't wait for any children so we just output L[0]
    asserted_write(1, L[0], sizeof(L[0]));
    word_index++;
    asserted_write(pds[index][1], &word_index, sizeof(word_index));
    output_word(0);

    exit(0);
}