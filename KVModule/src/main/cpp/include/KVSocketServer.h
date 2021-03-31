//
// Created by admin on 2021/3/31.
//

#ifndef DEMO_KVSOCKETSERVER_H
#define DEMO_KVSOCKETSERVER_H
#define EPOLL_MAX_NUM 8
#define LISTEN_BAK_NUM 5

class KVSocketServer {
public:
    static KVSocketServer& defaultKVServer();
    int init(char* addr, size_t addrLen, const char* dataDir);

    KVSocketServer();
    ~KVSocketServer();


    bool start();
    bool isExiting() const { return mIsExiting; }
    bool wait(int* exitStatus);
    bool tryWait(int* exitStatus);
    int threadLoop();

private:
    static void* main(void* args);
    void epollHandler();


    pthread_t mThread;
    pthread_mutex_t mLock;
    int mExitStatus;
    bool mIsRunning;
    bool mIsExiting;
    bool mForceExit;

    int mEpollFd;
    int mListenFd;
};


#endif //DEMO_KVSOCKETSERVER_H
