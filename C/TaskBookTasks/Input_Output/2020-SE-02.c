#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

int asserted_open(const char* filename, int mode, int* perms) {
    int fd;
    if (perms != NULL) {
        fd = open(filename, mode, *perms);
    }
    else {
        fd = open(filename, mode);
    }

    if (fd < 0) {
        err(2, "Failed to open file %s", filename);
    }

    return fd;
}

int asserted_read(int fd, void* buff, int size) {
    int r = read(fd, buff, size);
    if (r < 0) {
        err(2, "Failed to read from %d", fd);
    }
    return r;
}

int asserted_write(int fd, void* buff, int size) {
    int w = write(fd, buff, size);
    if (w < 0) {
        err(2, "Failed to write to %d", fd);
    }
    return w;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        err(1, "Expected 2 args");
    }

    int fdSCL = asserted_open(argv[1], O_RDONLY, NULL);
    int fdSDL = asserted_open(argv[2], O_RDONLY, NULL);
    int perms = S_IRUSR | S_IWUSR | S_IXUSR;
    int fdRes = asserted_open("res", O_CREAT | O_WRONLY | O_TRUNC, &perms);

    struct stat st1;
    struct stat st2;

    if (fstat(fdSCL, &st1) == -1) {
        err(3, "Failed to fstat scl");
    }

    if (fstat(fdSDL, &st2) == -1) {
        err(3, "Failed to fstat sdl");
    }

    if ((st1.st_size * 8) != (st2.st_size / 2)) {
        err(3, "SCL and SDL files sizes must match");
    }

    uint8_t bytes;
    while (asserted_read(fdSCL, &bytes, sizeof(bytes)) > 0) {
        for (int i = 0; i < 8; i++) {
            uint16_t sdl_element;
            asserted_read(fdSDL, &sdl_element, sizeof(sdl_element));

            if ((bytes >> i) & 1) {
                asserted_write(fdRes, &sdl_element, sizeof(sdl_element));
            }
        }
    }

    close(fdSCL);
    close(fdSDL);
    close(fdRes);

    return 0;
}