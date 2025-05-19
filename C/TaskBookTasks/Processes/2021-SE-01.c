#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include <stdio.h>
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

int asserted_write(int fd, const void* buff, int size) {
    int w = write(fd, buff, size);

    if (w < 0) {
        err(2, "Failed to write to file");
    }

    return w;
}

void lockUser(const char* username) {
    pid_t child_pid = fork();
    if (child_pid < 0) {
        err(3, "Failed to fork process");
    }

    if (child_pid == 0) {
        if (execlp("passwd", "passwd", "-l", username, (void*)NULL) < 0) {
            err(4, "Failed to exec passwd command");
        }
    }
}

int ps(const char* username) {
    int pd[2];
    if (pipe(pd) < 0) {
        err(4, "Err creating pipe");
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        err(3, "Failed to fork process");
    }

    if (child_pid == 0) {
        close(pd[0]);

        if (dup2(pd[1], 1) < 0) {
            err(4, "Failed to dup2");
        }
        close(pd[1]);

        if (execlp("ps", "ps", "-u", username, "-o", "pid=", (void*)NULL) < 0) {
            err(5, "Failed to exec command ps");
        }
        exit(0);
    }

    close(pd[1]);
    return pd[0];
}

int readNextProcess(int pipeInput, char buff[1024]) {
    int bytes = 0;
    char ch;
    int index = 0;
    while ((bytes = read(pipeInput, &ch, sizeof(ch))) > 0) {
        if (ch == ' ') {
            continue;
        }

        if (ch == '\n') {
            break;
        }

        buff[index++] = ch;
    }

    if (bytes < 0) {
        err(6, "Failed to read bytes from pipeInput");
    }

    if (bytes == 0) {
        return 0;
    }

    buff[index++] = '\0';
    return bytes;
}

void killProcess(int pid) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        err(3, "Failed to fork process");
    }

    if (child_pid == 0) {
        if (execlp("kill", "kill", pid, (void*)NULL) < 0) {
            err(4, "Failed to execute command kill");
        }
    }
}

int main(int argc, char* argv[]) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        err(1, "Couldn't run gettimeofday");
    }

    struct tm* tm = localtime(&tv.tv_sec);
    if (tm == NULL) {
        err(1, "Couldn't get local time");
    }

    char date[30];
    if (strftime(date, 30, "%F %T", tm) <= 0) {
        err(1, "Failed to format date");
    }

    uid_t uid = getuid();
    struct passwd* pwd = getpwuid(uid);
    if (pwd == NULL) {
        err(1, "Failed to extract passwd struct");
    }

    // Write new line to file foo.log
    int perms = S_IWUSR | S_IRUSR | S_IXUSR;
    int fd = asserted_open("foo.log", O_CREAT | O_APPEND | O_WRONLY, &perms);
    asserted_write(fd, date, strlen(date));
    asserted_write(fd, " ", 1);
    asserted_write(fd, pwd->pw_name, strlen(pwd->pw_name));
    asserted_write(fd, " ", 1);

    for (int i = 1; i < argc; i++) {
        asserted_write(fd, argv[i], strlen(argv[i])); // !!! write buff not as r
        asserted_write(fd, " ", 1);
    }

    asserted_write(fd, "\n", 1);

    // Lock user - ! don't execute it
    //lockUser(pwd->pw_name);

    // Finish the execution of all processes started from the user
    int pipeInput = ps(pwd->pw_name);
    char buff[1024];
    int bytesRead;
    while ((bytesRead = readNextProcess(pipeInput, buff)) > 0) {
        printf("PID to kill %s\n", buff);
        // killProcess(buff);
    }

    if (bytesRead < 0) {
        errx(5, "Failed to read from pd[0] = ps");
    }

    close(pipeInput);
    close(fd);
}