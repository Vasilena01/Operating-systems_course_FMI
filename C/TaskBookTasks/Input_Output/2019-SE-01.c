#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>

typedef struct {
    uint32_t id;
    uint16_t tmp1;
    uint16_t tmp2;
    uint32_t startTime;
    uint32_t endTime;
} UserInfo;

uint32_t getAvgValue(const UserInfo* users, size_t usersCount) {
    uint32_t avg = 0;
    for (size_t i = 0; i < usersCount; i++) {
        UserInfo curr = users[i];
        uint32_t duration = curr.endTime - curr.startTime;
        avg += duration;
    }

    return avg / usersCount;
}

uint32_t getD(const UserInfo* users, size_t usersCount, uint32_t avgValue) {
    uint32_t d = 0;
    for (size_t i = 0; i < usersCount; i++) {
        UserInfo curr = users[i];
        uint32_t duration = ((curr.endTime - curr.startTime) - avgValue) * ((curr.endTime - curr.startTime) - avgValue);
        d += duration;
    }

    return d / usersCount;
}

void getMaxDurationForUsers(uint32_t* ids, uint32_t* durations, const UserInfo* users, size_t usersCount, size_t* uniqueCount) {
    for (size_t i = 0; i < usersCount; i++) {
        UserInfo curr = users[i];
        uint32_t id = curr.id;
        uint32_t duration = curr.endTime - curr.startTime;
        size_t j;
        for (j = 0; j < *uniqueCount; j++) {
            if (ids[j] == id) {
                if (duration > durations[j]) {
                    durations[j] = duration;
                }
                break;
            }
        }

        // newId
        if (j == *uniqueCount) {
            ids[j] = id;
            durations[j] = duration;
            (*uniqueCount)++;
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        errx(1, "Expected one arg to be passed");
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        err(2, "Couldn't open file %s", argv[1]);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        err(2, "Couldn't fstat fd");
    }

    size_t fileSize = st.st_size;
    uint32_t ids[2048] = {0};
    uint32_t maxDurations[2048] = {0};
    size_t uniqueUsersCount = 0;
    UserInfo users[16384];
    int bytesCount;

    if ((bytesCount = read(fd, users, fileSize)) < 0) {
        err(3, "Couldn't read users info from file");
    }

    size_t usersCount = bytesCount / sizeof(UserInfo);
    getMaxDurationForUsers(ids, maxDurations, users, usersCount, &uniqueUsersCount);
    uint32_t avgValue = getAvgValue(users, usersCount);
    uint32_t D = getD(users, usersCount, avgValue);

    for (size_t i = 0; i < uniqueUsersCount; i++) {
        uint32_t id = ids[i];
        uint32_t maxDuration = maxDurations[i];

        if ((maxDuration * maxDuration) > D) {
            if (write(1, &id, sizeof(id)) != sizeof(id)) {
                err(3, "Couldn't write id to standart output");
            }

            if (write(1, &maxDuration, sizeof(maxDuration)) != sizeof(maxDuration)) {
                err(3, "Couldn't write maxDuration to standart output");
            }
        }
    }
    
    close(fd);
}
