//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_STRTAB_H
#define DEMO_STRTAB_H
typedef struct strtab_info {
    size_t nameLen;
    size_t valueLen;
    char data[0];
} strtab_info;

struct strtab_area {
    size_t tab_size;
    size_t bytes_used;
    size_t data_size;
    pthread_mutex_t mLock;

    union {
        char data[0];
        strtab_info *arr;
    };

    strtab_area(size_t mapSize) {
        pthread_mutex_init(&mLock, NULL);
        tab_size = mapSize;
        bytes_used = 0;
        data_size = tab_size - sizeof(strtab_area);
    }

    void clear() {
        pthread_mutex_destroy(&mLock);
    }

    void lock() {
        pthread_mutex_lock(&mLock);
    }

    void unlock() {
        pthread_mutex_unlock(&mLock);
    }

    DISALLOW_COPY_AND_ASSIGN(strtab_area);
};
#endif //DEMO_STRTAB_H
