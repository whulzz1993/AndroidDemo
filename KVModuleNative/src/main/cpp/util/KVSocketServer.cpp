//
// Created by liuzhuangzhuang.das on 2020/12/15.
//


#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <stdio.h>
#include <kv.h>

#define LOG_TAG "KVSServer"
#include "log.h"
#include "ScopedBuffer.h"
#include "utils.h"
#include "ScopedMutexLock.h"
#include "KVSocketServer.h"

KVSocketServer& KVSocketServer::defaultKVServer() {
    static KVSocketServer server;
    return server;
}

int KVSocketServer::init(char *addr, size_t addrLen, const char* dataDir) {
    int fd = epoll_create(EPOLL_MAX_NUM);
    if (fd == -1) {
        LOGE("epoll creation failed: %s\n", strerror(errno));
        return 0;
    }
    mEpollFd = fd;


    ScopedBuffer<char, UNIX_PATH_MAX> dirBuffer;
    ScopedBuffer<char, UNIX_PATH_MAX> unixPathBuffer;
    snprintf(dirBuffer.buffer(), UNIX_PATH_MAX, SERVICE_DIR, dataDir);

    if (make_unix_path(dirBuffer.buffer(), unixPathBuffer.buffer(),
            UNIX_PATH_MAX, SERVICE_PREFIX, SERVICE_PORT) < 0) {
        LOGE("make socket path failed: %s\n", strerror(errno));
        return 0;
    }

    unlink(unixPathBuffer.buffer());
    fd = socket_local_bind(unixPathBuffer.buffer(), SOCK_STREAM);

    if (fd == -1) {
        LOGE("socket bind failed: %s\n", strerror(errno));
        return 0;
    }
    mListenFd = fd;
    size_t len = strlen(unixPathBuffer.buffer()) + 1;
    if (len > addrLen) {
        LOGE("address name too big for provided buffer: %zu > %zu\n",
             len, addrLen);
        return 0;
    }

    if (listen(mListenFd, LISTEN_BAK_NUM) == -1) {
        LOGE("socket listen failed: %s\n", strerror(errno));
        return 0;
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = nullptr;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mListenFd, &ev) == -1) {
        LOGE("epoll_ctl failed: %s\n", strerror(errno));
        return 0;
    }

    memcpy(addr, unixPathBuffer.buffer(), len);
    return 1;
}

KVSocketServer::KVSocketServer() :
        mThread((pthread_t)NULL),
        mExitStatus(0),
        mIsRunning(false),
        mIsExiting(false),
        mForceExit(false),
        mEpollFd(0),
        mListenFd(0) {
    pthread_mutex_init(&mLock, NULL);
}

KVSocketServer::~KVSocketServer() {
    if (mEpollFd > 0) close(mEpollFd);
    if (mListenFd > 0) close(mListenFd);
    pthread_mutex_destroy(&mLock);
}

bool KVSocketServer::start() {
    ScopedMutexLock _lock(&mLock);
    mIsRunning = true;
    int ret = pthread_create(&mThread, NULL, main, this);
    if(ret) {
        mIsRunning = false;
    }
    return mIsRunning;
}

bool KVSocketServer::wait(int* exitStatus) {
    if (!mIsRunning) {
        return false;
    }

    void *ret;
    if (pthread_join(mThread,&ret)) {
        return false;
    }

    if (exitStatus) {
        *exitStatus = (int)(uintptr_t)ret;
    }
    return true;
}

bool KVSocketServer::tryWait(int* exitStatus) {
    ScopedMutexLock _lock(&mLock);
    bool ret = false;
    if (!mIsRunning) {
        *exitStatus = mExitStatus;
        ret = true;
    }
    return ret;
}

void* KVSocketServer::main(void *arg) {
    KVSocketServer* self = (KVSocketServer *)arg;
    int ret = self->threadLoop();

    ScopedMutexLock _lock(&self->mLock);
    self->mIsRunning = false;
    self->mExitStatus = ret;
    return (void*)(uintptr_t)ret;
}

int KVSocketServer::threadLoop() {
    while (true) {
        epoll_event ev;
        int nr = TEMP_FAILURE_RETRY(epoll_wait(mEpollFd, &ev, 1, -1));
        if (nr == -1) {
            LOGE("epoll_wait failed: %s\n", strerror(errno));
        } else if (nr == 1) {
            epollHandler();
        }
        if (mForceExit) break;
    }
    return 0;
}

void KVSocketServer::epollHandler() {
    int s;
    int r;
    struct ucred cr;
    struct sockaddr_un addr;
    socklen_t addr_size = sizeof(addr);
    socklen_t cr_size = sizeof(cr);
    struct pollfd ufds[1];
    const int timeout_ms = 1 * 1000;
    int nr;

    if ((s = accept(mListenFd, (struct sockaddr *) &addr, &addr_size)) < 0) {
        return;
    }

    /* Check socket options here */
    if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
        close(s);
        LOGE("unable to receive socket options\n");
        return;
    }

    ufds[0].fd = s;
    ufds[0].events = POLLIN;
    ufds[0].revents = 0;
    nr = TEMP_FAILURE_RETRY(poll(ufds, 1, timeout_ms));
    if (nr == 0) {
        LOGE("timeout waiting for uid=%d to send stack message.\n", cr.uid);
        close(s);
        return;
    } else if (nr < 0) {
        LOGE("error waiting for uid=%d to send stack message: %s\n", cr.uid, strerror(errno));
        close(s);
        return;
    }

    struct msghdr hdr;
    r = TEMP_FAILURE_RETRY(recvmsg(s, &hdr, MSG_DONTWAIT));
    if (r != sizeof(hdr)) {
        LOGE("mis-match msg size received: %d expected: %zu: %s\n",
             r, sizeof(kv_msg), strerror(errno));
        close(s);
        return;
    }
    struct iovec* iov = hdr.msg_iov;
    size_t iovLen = hdr.msg_iovlen;
    if (iovLen <= 0 || !iov || iov->iov_len <= 0 || !iov->iov_base) {
        LOGE("iovec invalid");
        close(s);
        return;
    }
    kv_msg* msg = (kv_msg*)iov->iov_base;

    switch (msg->cmd) {
        case MSG_SET_VALUE:
            if (strlen(msg->key) < 1) {
                LOGE("illegal path\n");
                close(s);
                return;
            }
            server_set_kv(msg->key, msg->value);
            close(s);
            break;
        default:
            close(s);
            break;
    }
}