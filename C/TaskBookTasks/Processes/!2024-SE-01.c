#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>
#include <sys/wait.h>

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;

    if (perms != NULL) {
        fd = open(filename, mode, perms);
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
        err(3, "Failed to read info from file");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(3, "Failed to write info to file");
    }

    return w;
}

void readBytesFromFile(int fd, char* bytes, long S) {
    char ch;
    for (long i = 0; i < S; i++) {
        asserted_read(fd, &ch, sizeof(ch));
        bytes[i] = ch;
    }

    bytes[S] = '\0';
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        err(1, "Expected 3 args");
    }

    const char* testedProgram = argv[1];
    int N = atoi(argv[2]);
    const char* resultFile = argv[3];

    int fdVirtual = asserted_open("/dev/urandom", O_RDONLY, NULL);
    int fdDevNull = asserted_open("/dev/null", O_WRONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdResult = asserted_open(resultFile, O_CREAT | O_TRUNC | O_RDWR, &perms);
    bool hasCrashed = false;

    // Run the program N times
    for (int i = 0; i < N; i++) {
        // Creating a pipe, so to dup2 pd[1], 1 - no output from exec to be shown to 1
        int pd[2];
        if (pipe(pd) < 0) {
            err(4, "Failed to pipe");
        }

        pid_t child_pid = fork();
        if (child_pid < 0) {
            err(3, "Failed to fork");
        }

        if (child_pid == 0) {
            close(pd[1]);

            // Making the stdin of the program to be pd[0], so it can read the r
            if (dup2(pd[0], 0) < 0) {
                err(6, "Failed to dup2");
            }

            close(pd[0]);

            // Redirecting stdout and stderr to go to /dev/null
            if (dup2(fdDevNull, 2) < 0) {
                err(6, "Failed to dup2");
            }

            if (dup2(fdDevNull, 1) < 0) {
                err(6, "Failed to dup2");
            }

            close(fdDevNull);

            if (execlp(testedProgram, testedProgram, (void*)NULL) < 0) {
                err(4, "Failed to execute command");
            }
        }

        close(pd[0]);

        // Pass S bytes to the input of the command = to the child process via the pipe
        uint16_t S;
        asserted_read(fdVirtual, &S, sizeof(S));

        // Read S count bytes from virtual file
        char buff[65536];
        readBytesFromFile(fdVirtual, buff, S);

        // Pass the read bytes to pd[1] - to child process
        asserted_write(pd[1], buff, S);

        int status;
        if (wait(&status) < 0) {
            err(5, "Failed to wait child process");
        }

        // There was a signal from the process, so we write to resultFile
        if (WIFSIGNALED(status)) {
            asserted_write(fdResult, buff, sizeof(buff));
            hasCrashed = true;
        }
    }

    close(fdVirtual);
    close(fdDevNull);
    close(fdResult);

    if (hasCrashed) {
        exit(42);
    }
    else {
        exit(0);
    }
}
