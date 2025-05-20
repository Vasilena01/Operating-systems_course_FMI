// First valid solution: 
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>

// Creating an array of two pipes, pd[0] - child pipe and pd[1] - parent pipe
int pd[2][2];

void close_all(void) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            close(pd[i][j]);
        }
    }
}

void wait_child(void) {
    int status;
    if (wait(&status) == -1) {
        err(6, "Could not wait for child process to finish");
    }

    if (!WIFEXITED(status)) {
        errx(7, "Child process did not terminate normally");
    }

    if (WEXITSTATUS(status) != 0) {
        errx(8, "Child process finished with exit code not 0");
    }
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int N = atoi(argv[1]);
    int D = atoi(argv[2]);

    if (N < 0 || N > 10) {
        err(2, "N should be between 0-9");
    }

    if (D < 0 || D > 10) {
        err(2, "D should be between 0-9");
    }

    // Creating an array of pipes - the child to have an input and output
    for (int i = 0; i < 2; i++) {
        if (pipe(pd[i]) < 0) {
            err(3, "Failed to pipe");
        }
    }

    pid_t child_pid = fork();

    if (child_pid < 0) {
        err(3, "Failed to fork process");
    }

    for (int i = 0; i < N; i++) {
        if (child_pid == 0) {
            close(pd[1][0]);
            close(pd[0][1]);

            // Before we write DONG we need to be sure that the parent has written DING
            bool flag;
            if (read(pd[0][0], &flag, sizeof(flag)) < 0) {
                err(5, "Failed to read from pipe's input");
            }

            write(1, "DONG\n", 5);

            // Signals the parent process that it has written DONG
            dprintf(pd[1][1], "%d", 0);

            if (i == N - 1) {
                exit(0);
            }
        }
        else {
            close(pd[0][0]);
            close(pd[1][1]);

            write(1, "DING\n", 5);
            dprintf(pd[0][1], "%d", 1);

            bool flag;
            if (read(pd[1][0], &flag, sizeof(flag)) < 0) {
                err(5, "Failed to read from pipe's input");
            }

            // This means that the child process has already written DONG
            sleep(D);
        }
    }

    wait_child();
    close_all();
}

// Second solution - almost valid (it works, but on every iteration it creates a new child
// and uses just one pipe, or wait between the parent and current child process, but this sol.
// isn't following the task reqs for one parent and one child)

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int N = atoi(argv[1]);
    int D = atoi(argv[2]);

    if (N < 0 || N > 10) {
        err(2, "N should be between 0-9");
    }

    if (D < 0 || D > 10) {
        err(2, "D should be between 0-9");
    }

    for (int i = 0; i < N; i++) {
        int pd[2];
        if (pipe(pd) < 0) {
            err(3, "Failed to pipe");
        }

        // Before the fork we execute DING
        write(1, "DING\n", 5);

        pid_t child_pid = fork();

        if (child_pid < 0) {
            err(3, "Failed to fork process");
        }

        if (child_pid == 0) {
            close(pd[0]);
            write(1, "DONG\n", 5);
            // Sygnals the parent process that it has written DONG
            dprintf(pd[1], "%d", 1);
            exit(0);
        }

        // Parent process - parent process can wait also with wait(&status) for
        // it's not necessary to make the "waiting" with pipe
        //int status;
        //wait(&status);

        close(pd[1]);
        bool flag;
        if (read(pd[0], &flag, sizeof(flag)) < 0) {
            err(5, "Failed to read from pipe's input");
        }

        // This means that the child process has already written DONG
        if (flag == 1) {
            sleep(D);
        }
        else {
            err(6, "Parent process got an invalid tag from child process");
        }
    }
}