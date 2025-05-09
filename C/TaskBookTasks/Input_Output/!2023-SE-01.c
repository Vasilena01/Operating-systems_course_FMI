// This is my solution
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        errx(1, "Expected 2 args");
    }

    int fd1 = open(argv[1], O_RDONLY);
    if (fd1 == -1) {
        err(2, "Couldn't open file %s", argv[1]);
    }

    int fd2 = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd2 == -1) {
        err(2, "Couldn't open file %s", argv[2]);
    }

    uint8_t currByte;
    uint8_t N = 0;
    uint8_t readBytesCount = 0;
    uint8_t checksum = 0;
    int count = 0;

    while ((count = read(fd1, &currByte, sizeof(currByte))) == sizeof(currByte)) {
        if (currByte == 0x55 && readBytesCount == 0) {
            if (write(fd2, &currByte, sizeof(currByte)) != sizeof(currByte)) {
                err(3, "Couldn't write first byte to file %s", argv[2]);
            }
            readBytesCount++;
            checksum = 0;  // reset checksum for new message
        }
        else if (readBytesCount == 1) {
            N = currByte;
            if (N < 3) {  // minimum valid message length (0x55 + N + checksum + at least 1 data byte)
                readBytesCount = 0;
                continue;
            }

            if (write(fd2, &N, sizeof(N)) != sizeof(N)) {
                err(3, "Couldn't write number N to file %s", argv[2]);
            }
            readBytesCount++;
            checksum = 0;  // reset checksum after N
        }
        else if (readBytesCount >= 2 && readBytesCount < N - 1) {
            if (write(fd2, &currByte, sizeof(currByte)) != sizeof(currByte)) {
                err(3, "Couldn't write byte from sequence to file %s", argv[2]);
            }

            checksum ^= currByte;
            readBytesCount++;
        }
        else if (readBytesCount == N - 1) {
            if (currByte != checksum) {
                // invalid message, discard
                readBytesCount = 0;
                continue;
            }

            if (write(fd2, &currByte, sizeof(currByte)) != sizeof(currByte)) {
                err(3, "Couldn't write last byte to file %s", argv[2]);
            }

            readBytesCount = 0;
        }
        else {
            readBytesCount = 0;
            continue;
        }
    }

    if (count == -1) {
        err(4, "Couldn't read correctly bytes from file %s", argv[1]);
    }

    close(fd1);
    close(fd2);
}

// This is another better solution
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>

uint8_t check(const uint8_t* buff, uint8_t size);
uint8_t check(const uint8_t* buff, uint8_t size) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < size; i++) {
        res ^= buff[i];
    }
    return res;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        errx(1, "2 args needed");
    }

    int ifd = open(argv[1], O_RDONLY);
    if (ifd < 0) {
        err(2, "err opening input file");
    }

    int ofd = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
    if (ofd < 0) {
        err(3, "err creating output file");
    }

    uint8_t buff[256];
    uint8_t byte;
    uint8_t N = 0;
    uint8_t checksum = 0;

    int readbyte;
    while ((readbyte = read(ifd, &byte, sizeof(byte))) == sizeof(byte)) {

        if (byte == 0x55) {
            buff[0] = byte;

            if (read(ifd, &N, sizeof(N)) != sizeof(N)) {
                err(4, "err reading N");
            }

            buff[1] = N;
            if (N < 3) {
                errx(5, "N should be > 3");
            }

            for (int i = 2; i < N - 1; i++) {

                if ((readbyte = read(ifd, &buff[i], sizeof(buff[i]))) != sizeof(buff[i])) {
                    if (readbyte == 0) {
                        err(6, "reached end before end of N");
                    }
                    err(6, "err reading buff");
                }
            }

            if (read(ifd, &checksum, sizeof(checksum)) != sizeof(checksum)) {
                err(7, "err reading checksum");
            }

            if (checksum == check(buff, N - 1)) {
                buff[N - 1] = checksum;

                if (write(ofd, &buff, N) != N) {
                    err(8, "err writing valid message");
                }
            }
        }

    }
    close(ifd);
    close(ofd);
    return 0;
}