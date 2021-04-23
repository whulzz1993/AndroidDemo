//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_FUTEX_H
#define DEMO_FUTEX_H
#include <linux/futex.h>
#include <syscall.h>
#include <errno.h>
static inline __always_inline int __futex(volatile void* ftx, int op, int value,
                                          const struct timespec* timeout,
                                          int bitset) {
    // Our generated syscall assembler sets errno, but our callers (pthread functions) don't want to.
    int saved_errno = errno;
    int result = syscall(__NR_futex, ftx, op, value, timeout, NULL, bitset);
    if (__predict_false(result == -1)) {
        result = -errno;
        errno = saved_errno;
    }
    return result;
}

static inline int __futex_wake(volatile void* ftx, int count) {
    return __futex(ftx, FUTEX_WAKE, count, NULL, 0);
}

static inline int __futex_wait(volatile void* ftx, int value, const struct timespec* timeout) {
    return __futex(ftx, FUTEX_WAIT, value, timeout, 0);
}
#endif //DEMO_FUTEX_H
