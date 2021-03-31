//
// Created by Admin on 2020/8/10
//
#include <stdint.h>
#include <time.h>
uint64_t gettime_ns() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000000) + now.tv_nsec;
}