#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/wait.h>

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);

    if (r < 0) {
        err(2, "Couldn't read from file");
    }

    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Couldn't write to file");
    }

    return w;
}

void executeEcho(void) {
    char ch;
    int bytesRead;
    while ((bytesRead = asserted_read(0, (void*)&ch, sizeof(ch))) > 0) {
        asserted_write(1, (void*)&ch, sizeof(ch));
    }

    if (bytesRead < 0) {
        err(2, "Couldn't read bytes from std input");
    }
}

void executeOther(const char* command) {

    char ch;
    char buff[1024];
    int charCount = 0;
    int bytesRead;
    while ((bytesRead = asserted_read(1, (void*)&ch, sizeof(ch))) > 0) {

        if (ch == ' ' || ch == '\n') {
            buff[charCount] = '\0';
            charCount = 0;

            if (strlen(buff) > 0 && strlen(buff) <= 4) {
                pid_t child_pid = fork();

                if (child_pid == -1) {
                    err(4, "Failed to fork process");
                }

                if (child_pid == 0) {
                    if (execlp(command, command, buff, (void*)NULL) < 0) {
                        err(5, "Failed to execute command");
                    }
                }

                int status;
                if (wait(&status) < 0) {
                    err(6, "Couldn't wait for the child process");
                }

                if (!WIFEXITED(status)) {
                    err(7, "Failed to exit properly");
                }

                if (WEXITSTATUS(status) != 0) {
                    err(8, "Command exited with status != 0");
                }
            }
            else {
                err(3, "Param len should be between 0-4 chars");
            }
        }
        else {
            buff[charCount] = ch;
            charCount++;
        }

        if (bytesRead == -1) {
            err(2, "Couldn't read from stdin");
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc == 1) {
        executeEcho();
    }
    else if (argc == 2) {
        if (strlen(argv[1]) > 4) {
            err(1, "Command len should be max 4 chars");
        }

        executeOther(argv[1]);
    }
    else {
        errx(1, "Expected 0 or 1 args");
    }
}
