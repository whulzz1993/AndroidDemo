//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_SCOPEDMUTEXLOCK_H
#define DEMO_SCOPEDMUTEXLOCK_H
#include <pthread.h>

class ScopedMutexLock {
        public:
        explicit ScopedMutexLock(pthread_mutex_t* mutex) : mMutexPtr(mutex) {
            pthread_mutex_lock(mMutexPtr);
        }

        ~ScopedMutexLock() {
            pthread_mutex_unlock(mMutexPtr);
        }

        private:
        pthread_mutex_t* mMutexPtr;

        // Disallow copy and assignment.
        ScopedMutexLock(const ScopedMutexLock&);
        void operator=(const ScopedMutexLock&);
};
#endif //DEMO_SCOPEDMUTEXLOCK_H
