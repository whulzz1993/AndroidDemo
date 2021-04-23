//
// Created by Admin on 2020/8/10
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>
#include <libgen.h>

#include "log.h"
#include "utils.h"
#include "ScopedBuffer.h"
int get_module_full_path(pid_t pid, const char* module_name, char *out)
{
    FILE *fp;
    int ret = -1;
    ScopedBuffer<char, 32> filename_buf;
    char *filename = filename_buf.buffer();
    ssize_t filename_len = filename_buf.size();
    ScopedBuffer<char, 1024> line_buf;
    char *line = line_buf.buffer();
    ssize_t line_len = line_buf.size();

    if (pid < 0) {
        /* self process */
        snprintf(filename, filename_len, "/proc/self/maps");
    } else {
        snprintf(filename, filename_len, "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, line_len, fp)) {
            if (strstr(line, module_name)) {
                char *start, *end;
                if ((start = strchr(line, '/')) && (end = strrchr(line, '/'))
                    && !strncmp(end + 1, module_name, strlen(module_name))) {
                    end = strrchr(line, '\n');
                    if (out && end) {
                        strncpy(out, start, end - start);
                        out[end-start] = 0;
                        ret = 0;
                        break;
                    }
                }
            }
        }
        fclose(fp) ;
    }

    return ret;
}

int get_module_full_path_from_addr(pid_t pid, void *addr, char *out)
{
    FILE *fp;
    int ret = -1;
    ScopedBuffer<char, 32> filename_buf;
    char *filename = filename_buf.buffer();
    ssize_t filename_len = filename_buf.size();
    ScopedBuffer<char, 1024> line_buf;
    char *line = line_buf.buffer();
    ssize_t line_len = line_buf.size();

    if (pid < 0) {
        /* self process */
        snprintf(filename, filename_len, "/proc/self/maps");
    } else {
        snprintf(filename, filename_len, "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, line_len, fp)) {
            char*end0 = (char*)0;
            unsigned long addr0 = strtoul(line, &end0, 0x10);
            char*end1 = (char*)0;
            unsigned long addr1 = strtoul(end0+1, &end1, 0x10);
            if ((void*)addr0 <= addr && addr < (void*)addr1) {
                char *start, *end;
                if ((start = strchr(line, '/')) && (end = strrchr(line, '\n'))) {
                    if (out) {
                        strncpy(out, start, end - start);
                        out[end-start] = 0;
                    }
                    ret = 0;
                    break;
                }
            }
        }
        fclose(fp) ;
    }

    return ret;
}

bool normalize_path(const char* path, char* const normalized_path) {
    if (path == nullptr || path[0] == '\0') {
        LOGE("normalize_path failed for null path!\n");
        return false;
    }

    if (path[0] == '.' && path[1] == '.') {
        LOGE("normalize_path failed for not root path('/') begin!");
        return false;
    }

    const size_t len = strlen(path) + 1;
    char buf[len];

    const char* in_ptr = path;
    char* out_ptr = buf;

    while (*in_ptr != 0) {
        if (*in_ptr == '/') {
            char c1 = in_ptr[1];
            if (c1 == '.') {
                char c2 = in_ptr[2];
                if (c2 == '/') {
                    in_ptr += 2;
                    continue;
                } else if (c2 == '.' && (in_ptr[3] == '/' || in_ptr[3] == 0)) {
                    in_ptr += 3;
                    while (out_ptr > buf && *--out_ptr != '/') {
                    }
                    if (in_ptr[0] == 0) {
                        out_ptr++;
                    }
                    continue;
                }
            } else if (c1 == '/') {
                ++in_ptr;
                continue;
            }
        }
        *out_ptr++ = *in_ptr++;
    }

    *out_ptr = 0;
    //if not '/'， ignore the end char '/'
    const size_t reallen = strlen(buf);
    if (reallen > 1 && buf[reallen - 1] == '/') {
        buf[reallen - 1] = 0;
    }

    strncpy(normalized_path, buf, len);
    return true;
}

bool file_is_in_dir(const char* file, const char* dir) {
    size_t dir_len = strlen(dir);

    return strncmp(file, dir, dir_len) == 0 &&
           file[dir_len] == '/' &&
           strchr(file + dir_len + 1, '/') == nullptr;
}

bool file_is_under_dir(const char* file, const char* dir) {
    size_t dir_len = strlen(dir);

    return strncmp(file, dir, dir_len) == 0 &&
           file[dir_len] == '/';
}

int make_unix_path(char *dir, char* outBuffer, size_t  pathlen, const char* prefix, int port_number)
{
    int ret = 0;

    // First, create user-specific temp directory if needed
    if (dir != NULL && *dir != 0) {
        struct stat  st;
        do {
            ret = lstat(dir, &st);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0 && errno == ENOENT) {
            do {
                ret = mkdir(dir, 0766);
            } while (ret < 0 && errno == EINTR);
            if (ret < 0) {
                LOGE("make_unix_path create directory %s failed!", dir);
            }
        }
        else if (ret < 0) {
            LOGE("make_unix_path create directory %s failed!", dir);
        }
    }

    if (ret >= 0) {
        snprintf(outBuffer, pathlen, "%s/%s-%d", dir, prefix, port_number);
    }
    return ret;
}

struct UnixAddr {
    socklen_t len;
    union {
        sockaddr generic;
        sockaddr_un local;
    };

    void initEmpty() {
        ::memset(this, 0, sizeof(*this));
        this->len = static_cast<socklen_t>(sizeof(*this));
    }

    int initFromUnixPath(const char *path) {
        if (!path || !path[0])
            return -EINVAL;

        size_t pathLen = strlen(path);
        if (pathLen >= sizeof(local.sun_path))
            return -E2BIG;

        memset(this, 0, sizeof(*this));
        this->local.sun_family = AF_LOCAL;
        memcpy(this->local.sun_path, path, pathLen + 1U);
        this->len = pathLen + offsetof(sockaddr_un, sun_path);
        return 0;
    }

    int getFamily() const {
        return AF_LOCAL;
    }
};

int socket_bind_internal(const UnixAddr* addr, int socketType) {
    int s = socket(addr->getFamily(), socketType, 0);
    if (s < 0)
        return -errno;

    int val = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind to the socket.
    if (bind(s, &addr->generic, addr->len) < 0) {
        LOGE("socket_bind_internal failed for: %s", strerror(errno));
        int ret = -errno;
        close(s);
        return ret;
    }
    return s;
}

int socket_local_bind(const char* path, int socketType) {
    UnixAddr addr;
    int ret = addr.initFromUnixPath(path);
    if (ret < 0) {
        return ret;
    }
    return socket_bind_internal(&addr, socketType);
}

int createDetachThread(thread_func_t entryFunction,
                              void* userData,
                              thread_id_t *threadId)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr, (thread_func_t)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        LOGE("createDetachThread failed! entry=%p, result=%d, errno=%d\n", entryFunction, result, errno);
        return 0;
    }

    if (threadId != NULL) {
        *threadId = (thread_id_t)thread;
    }
    return 1;
}

bool isPathCanWrite(const char* path) {

    struct stat st;
    int ret = 0;
    do {
        ret = access(path, F_OK);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0 && errno == ENOENT) {
        const char* parentDir = dirname(path);
        do {
            ret = access(parentDir, W_OK);
        } while (ret < 0 && errno == EINTR);
    } else {
        do {
            ret = access(path, W_OK);
        } while (ret < 0 && errno == EINTR);
    }
    return ret == 0;
}
