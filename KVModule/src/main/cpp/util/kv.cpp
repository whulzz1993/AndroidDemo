//
// Created by admin on 2021/3/22.
//
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <new>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <ctype.h>
#include <utils.h>
#include "cdefs.h"
#include "strtab.h"
#include "kv.h"
#include "ScopedMutexLock.h"
#include "ScopedBuffer.h"
#include "futex.h"
#include "timer.h"
#include <stdatomic.h>

#include "log.h"

struct prefix_node;
struct context_node;
static strtab_area* _sa_ = nullptr;
static char *sock_path = nullptr;
static char* kv_dir = nullptr;
static char* context_path = nullptr;
static bool initialized = false;
static prefix_node* prefixes = nullptr;
static context_node* contexts = nullptr;

struct kv_bt {
    uint32_t namelen;
    atomic_uint_least32_t info;
    atomic_uint_least32_t left;
    atomic_uint_least32_t right;
    atomic_uint_least32_t children;
    char name[0];

    kv_bt(const char *name, const uint32_t nameLen) {
        this->namelen = nameLen;
        memcpy(this->name, name, nameLen);
        this->name[nameLen] = '\0';
    }

private:
    DISALLOW_COPY_AND_ASSIGN(kv_bt);
};

static void *
allocate_strtab_info(size_t namelen, size_t valuelen, const char *name, const char *value,
                     uint_least32_t *off);

static strtab_info *to_strtab_info(const uint_least32_t off);

struct kv_info {
    atomic_uint_least32_t serial;
    uint_least32_t str_info;

    kv_info(const char *name, const uint32_t nameLen, const char *value,
            const uint32_t valueLen) {

        str_info = 0;
        strtab_info *info = nullptr;
        if (allocate_strtab_info(nameLen, valueLen, name, value, &str_info)) {
            info = to_strtab_info(str_info);
        }
        if (info != nullptr) {
            info->nameLen = nameLen;
            info->valueLen = valueLen;
            char *data = info->data;
            memcpy(data, name, nameLen);
            data += nameLen;
            *data++ = 0;
            atomic_init(&this->serial, valueLen << SERIAL_VALUE_SHIFT);
            memcpy(data, value, valueLen);
            data += valueLen;
            *data = 0;
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(kv_info);
};

class kv_area {
public:

    kv_area(const uint32_t magic, const uint32_t version, size_t pa_size) :
            magic_(magic), version_(version) {
        atomic_init(&serial_, (unsigned int) 0);
        pthread_mutex_init(&mLock, NULL);
        memset(reserved_, 0, sizeof(reserved_));
        bytes_used_ = 0;
        pa_data_size = pa_size - sizeof(kv_area);
    }

    size_t areaSize() const { return pa_size; }

    const kv_info *find(const char *name);

    bool add(const char *name, unsigned int namelen,
             const char *value, unsigned int valuelen);

    atomic_uint_least32_t *serial() { return &serial_; }

    uint32_t magic() const { return magic_; }

    uint32_t version() const { return version_; }

private:
    void *allocate_obj(const size_t size, uint_least32_t *const off);

    kv_bt *new_prop_bt(const char *name, uint32_t namelen, uint_least32_t *const off);

    kv_info *new_prop_info(const char *name, uint32_t namelen,
                           const char *value, uint32_t valuelen,
                           uint_least32_t *const off);

    void *to_prop_obj(uint_least32_t off);

    kv_bt *to_prop_bt(atomic_uint_least32_t *off_p);

    kv_info *to_prop_info(atomic_uint_least32_t *off_p);

    kv_bt *root_node();

    kv_bt *find_prop_bt(kv_bt *const bt, const char *name,
                        uint32_t namelen, bool alloc_if_needed);

    const kv_info *find_prop(kv_bt *const trie, const char *name,
                             uint32_t namelen, const char *value,
                             uint32_t valuelen, bool alloc_if_needed);

    bool foreach_prop(kv_bt *const trie,
                      void (*callback)(int type, const strtab_info *si, void *cookie),
                      void *cookie);

    size_t pa_size;
    size_t pa_data_size;
    uint32_t bytes_used_;
    pthread_mutex_t mLock;
    atomic_uint_least32_t serial_;
    uint32_t magic_;
    uint32_t version_;
    uint32_t reserved_[28];
    char data_[0];

    DISALLOW_COPY_AND_ASSIGN(kv_area);
};

static void *
allocate_strtab_info(size_t namelen, size_t valuelen, const char *name, const char *value,
                     uint_least32_t *off) {

    if (_sa_ == nullptr) return nullptr;
    _sa_->lock();
    size_t size = sizeof(strtab_info) + namelen + valuelen + 2;
    const size_t aligned = BYTE_ALIGN(size, sizeof(size_t));
    if (_sa_->bytes_used + aligned >= _sa_->tab_size) {
        LOGE("allocate_strtab_info failed! key: %s \n valuelen: %zu", name, valuelen);
        _sa_->unlock();
        return NULL;
    }

    *off = _sa_->bytes_used;
    _sa_->bytes_used += aligned;
#ifdef DEBUG_AREA
    LOGD("allocate_strtab_info strtab_size: %zu bytes_used: %zu", _sa_->tab_size, _sa_->bytes_used);
#endif
    _sa_->unlock();
    return off;
}

static strtab_info *to_strtab_info(const uint_least32_t off) {
    if (_sa_ == nullptr || off == 0) return nullptr;
    return reinterpret_cast<strtab_info *>(_sa_->data + off);
}

static inline size_t get_map_size(uint32_t type) {
    switch (type) {
        case TYPE_PA:
            return PA_SIZE;
        case TYPE_SA:
            return SA_SIZE;
        case TYPE_BIG:
            return BIG_SIZE;
        default:
            return PA_SIZE;
    }
}

static kv_area *map_area_rw(uint32_t type, const char *filename) {

    const int fd = open(filename, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_EXCL, 0444);

    if (fd < 0) {
        LOGD("map_area_rw %s failed for %s", filename, strerror(errno));
        return nullptr;
    }

    size_t mapSize = get_map_size(type);

    if (ftruncate(fd, mapSize) < 0) {
        close(fd);
        return nullptr;
    }

    void *const memory_area = mmap(NULL, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memory_area == MAP_FAILED) {
        close(fd);
        return nullptr;
    }

    kv_area *pa = new(memory_area) kv_area(PROP_AREA_MAGIC, PROP_AREA_VERSION, mapSize);

    close(fd);
    return pa;
}

static strtab_area *map_strtap_rw(size_t mapSize, const char *filename) {

    const int fd = open(filename, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_EXCL, 0444);

    if (fd < 0) {
        LOGD("map_strtap_rw failed for %s", strerror(errno));
        return nullptr;
    }

    if (ftruncate(fd, mapSize) < 0) {
        close(fd);
        return nullptr;
    }

    void *const memory_area = mmap(NULL, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memory_area == MAP_FAILED) {
        close(fd);
        return nullptr;
    }

    strtab_area *sa = new(memory_area) strtab_area(mapSize);

    close(fd);
    return sa;
}

static kv_area *map_fd_ro(const int fd) {
    struct stat fd_stat;
    if (fstat(fd, &fd_stat) < 0) {
        return nullptr;
    }

    if (((fd_stat.st_mode & (S_IWGRP | S_IWOTH)) != 0)
        || (fd_stat.st_size < static_cast<off_t>(sizeof(kv_area)))) {
        return nullptr;
    }

    size_t sa_size = fd_stat.st_size;

    void *const map_result = mmap(NULL, sa_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_result == MAP_FAILED) {
        return nullptr;
    }

    kv_area *pa = reinterpret_cast<kv_area *>(map_result);
    if ((pa->magic() != PROP_AREA_MAGIC) ||
        (pa->version() != PROP_AREA_VERSION)) {
        munmap(pa, sa_size);
        return nullptr;
    }
    return pa;
}

static strtab_area *map_strtab_ro(const int fd) {
    struct stat fd_stat;
    if (fstat(fd, &fd_stat) < 0) {
        return nullptr;
    }

    if (((fd_stat.st_mode & (S_IWGRP | S_IWOTH)) != 0)
        || (fd_stat.st_size < static_cast<off_t>(sizeof(strtab_area)))) {
        return nullptr;
    }

    size_t sa_size = fd_stat.st_size;

    void *const map_result = mmap(NULL, sa_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_result == MAP_FAILED) {
        return nullptr;
    }

    return reinterpret_cast<strtab_area *>(map_result);
}

static kv_area *map_prop_area(const char *filename) {
    int fd = open(filename, O_CLOEXEC | O_NOFOLLOW | O_RDONLY);

    if (fd < 0) {
        return nullptr;
    }

    kv_area *map_result = map_fd_ro(fd);

    close(fd);
    return map_result;
}

static strtab_area *map_strtab(const char *filename) {
    int fd = open(filename, O_CLOEXEC | O_NOFOLLOW | O_RDONLY);

    if (fd < 0) {
        return nullptr;
    }

    strtab_area *map_result = map_strtab_ro(fd);

    close(fd);
    return map_result;
}

void *kv_area::allocate_obj(const size_t size, uint_least32_t *const off) {
    ScopedMutexLock _lock(&mLock);
    const size_t aligned = BYTE_ALIGN(size, sizeof(uint_least32_t));
    if (bytes_used_ + aligned >= pa_data_size) {
        LOGE("allocate_obj failed!");
        return NULL;
    }

    *off = bytes_used_;
    bytes_used_ += aligned;
#ifdef DEBUG_AREA
    LOGD("allocate_obj pa_data_size: %zu bytes_used: %d", pa_data_size, bytes_used_);
#endif
    return data_ + *off;
}

kv_info *kv_area::new_prop_info(const char *name, uint32_t namelen,
                                const char *value, uint32_t valuelen,
                                uint_least32_t *const off) {
    uint_least32_t new_offset;
    void *const p = allocate_obj(sizeof(kv_info), &new_offset);
    if (p != NULL) {
        kv_info *info = new(p) kv_info(name, namelen, value, valuelen);
        *off = new_offset;
        return info;
    }
    return NULL;
}

kv_bt *kv_area::new_prop_bt(const char *name, uint32_t namelen, uint_least32_t *const off) {
    uint_least32_t new_offset;
    void *const p = allocate_obj(sizeof(kv_bt) + namelen + 1, &new_offset);
    if (p != NULL) {
        kv_bt *pt = new(p) kv_bt(name, namelen);
        *off = new_offset;
        return pt;
    }

    return NULL;
}

void *kv_area::to_prop_obj(uint_least32_t off) {
    if (off > pa_data_size)
        return NULL;

    return (data_ + off);
}

inline kv_bt *kv_area::to_prop_bt(atomic_uint_least32_t *off_p) {
    uint_least32_t off = atomic_load_explicit(off_p, memory_order_consume);
    return reinterpret_cast<kv_bt *>(to_prop_obj(off));
}

inline kv_info *kv_area::to_prop_info(atomic_uint_least32_t *off_p) {
    uint_least32_t off = atomic_load_explicit(off_p, memory_order_consume);
    return reinterpret_cast<kv_info *>(to_prop_obj(off));
}

inline kv_bt *kv_area::root_node() {
    return reinterpret_cast<kv_bt *>(to_prop_obj(0));
}

static int cmp_info_name(const char *one, uint32_t one_len, const char *two, uint32_t two_len) {
    if (one_len < two_len) {
        return -1;
    } else if (one_len > two_len) {
        return 1;
    } else {
        return strncmp(one, two, one_len);
    }
}

kv_bt *kv_area::find_prop_bt(kv_bt *const pt, const char *name,
                             uint32_t namelen, bool alloc_if_needed) {

    kv_bt *current = pt;
    while (true) {
        if (!current) {
            return NULL;
        }

        const int ret = cmp_info_name(name, namelen, current->name, current->namelen);
        if (ret == 0) {
            return current;
        }

        if (ret < 0) {
            uint_least32_t left_offset = atomic_load_explicit(&current->left, memory_order_relaxed);
            if (left_offset != 0) {
                current = to_prop_bt(&current->left);
            } else {
                if (!alloc_if_needed) {
                    return NULL;
                }

                uint_least32_t new_offset;
                kv_bt *new_pt = new_prop_bt(name, namelen, &new_offset);
                if (new_pt) {
                    atomic_store_explicit(&current->left, new_offset, memory_order_release);
                }
                return new_pt;
            }
        } else {
            uint_least32_t right_offset = atomic_load_explicit(&current->right,
                                                               memory_order_relaxed);
            if (right_offset != 0) {
                current = to_prop_bt(&current->right);
            } else {
                if (!alloc_if_needed) {
                    return NULL;
                }

                uint_least32_t new_offset;
                kv_bt *new_pt = new_prop_bt(name, namelen, &new_offset);
                if (new_pt) {
                    atomic_store_explicit(&current->right, new_offset, memory_order_release);
                }
                return new_pt;
            }
        }
    }
}

const kv_info *kv_area::find_prop(kv_bt *const trie, const char *name,
                                  uint32_t namelen, const char *value,
                                  uint32_t valuelen, bool alloc_if_needed) {

    if (!trie) return NULL;

    const char *remaining_name = name;
    kv_bt *current = trie;
    while (true) {
        const char *sep = strchr(remaining_name, '/');
        const bool want_subtree = (sep != NULL);
        const uint32_t substr_size = (want_subtree) ?
                                     sep - remaining_name : strlen(remaining_name);

        if (!substr_size) {
            return NULL;
        }

        kv_bt *root = NULL;
        uint_least32_t children_offset = atomic_load_explicit(&current->children,
                                                              memory_order_relaxed);

        if (children_offset != 0) {
            root = to_prop_bt(&current->children);
        } else if (alloc_if_needed) {
            uint_least32_t new_offset;
            root = new_prop_bt(remaining_name, substr_size, &new_offset);
            if (root) {
                atomic_store_explicit(&current->children, new_offset, memory_order_release);
            }
        }
#ifdef MATCH_PREFIX
        else if (children_offset == 0
                   && (type_ == TYPE_MATCH_PREFIX)) {
            break;
        }
#endif

        if (!root) {
            return NULL;
        }

        current = find_prop_bt(root, remaining_name, substr_size, alloc_if_needed);
        if (!current) {
            return NULL;
        }

        if (!want_subtree)
            break;

        remaining_name = sep + 1;
    }

    uint_least32_t stack_offset = atomic_load_explicit(&current->info, memory_order_relaxed);
    if (stack_offset != 0) {
        return to_prop_info(&current->info);
    } else if (alloc_if_needed) {
        uint_least32_t new_offset;
        kv_info *new_info = new_prop_info(name, namelen, value, valuelen, &new_offset);
        if (new_info) {
            atomic_store_explicit(&current->info, new_offset, memory_order_release);
        }

        return new_info;
    } else {
        return NULL;
    }
}

static int send_stack_msg(const kv_msg *msg, size_t msgLen) {
    const int fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd == -1) {
        return -1;
    }

    const size_t namelen = strlen(sock_path);

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    strlcpy(addr.sun_path, sock_path, sizeof(addr.sun_path));
    addr.sun_family = AF_LOCAL;
    socklen_t alen = namelen + offsetof(sockaddr_un, sun_path) + 1;
    if (TEMP_FAILURE_RETRY(connect(fd, reinterpret_cast<sockaddr *>(&addr), alen)) < 0) {
        close(fd);
        return -1;
    }

    struct msghdr hdr;
    struct iovec iov;
    hdr.msg_name = NULL;
    hdr.msg_namelen = 0;
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_flags = 0;
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;
    iov.iov_base = (void *) msg;
    iov.iov_len = msgLen;


    const int num_bytes = TEMP_FAILURE_RETRY(sendmsg(fd, &hdr, 0));

    int result = -1;
    if (num_bytes == sizeof(hdr)) {
        pollfd pollfds[1];
        pollfds[0].fd = fd;
        pollfds[0].events = 0;
        const int poll_result = TEMP_FAILURE_RETRY(poll(pollfds, 1, 250 /* ms */));
        if (poll_result == 1 && (pollfds[0].revents & POLLHUP) != 0) {
            result = 0;
        } else {
            // Ignore the timeout and treat it like a success anyway.
            result = 0;
        }
    }

    close(fd);
    return result;
}

const kv_info *kv_area::find(const char *name) {
    return find_prop(root_node(), name, strlen(name), nullptr, 0, false);
}

bool kv_area::add(const char *path, unsigned int pathLen,
                  const char *stack, unsigned int stackLen) {
    return find_prop(root_node(), path, pathLen, stack, stackLen, true);
}

class context_node {
public:
    context_node(context_node *next, const char *context, kv_area *pa)
            : next(next), context_(strdup(context)), pa_(pa), no_access_(false) {
        pthread_mutex_init(&mLock, NULL);
    }

    ~context_node() {
        unmap();
        free(context_);
        pthread_mutex_destroy(&mLock);
    }

    bool open(bool access_rw);

    bool check_access_and_open();

    void reset_access();

    const char *context() const { return context_; }

    kv_area *pa() { return pa_; }

    context_node *next;

private:
    bool check_access();

    void unmap();

    pthread_mutex_t mLock;
    char *context_;
    kv_area *pa_;
    bool no_access_;
};

struct prefix_node {
    prefix_node(struct prefix_node *next, const char *prefix, context_node *context)
            : prefix(strdup(prefix)), prefix_len(strlen(prefix)), context(context), next(next) {
    }

    ~prefix_node() {
        free(prefix);
    }

    char *prefix;
    const size_t prefix_len;
    context_node *context;
    struct prefix_node *next;
};

template<typename List, typename... Args>
static inline void list_add(List **list, Args... args) {
    *list = new List(*list, args...);
}

static void
list_add_after_len(prefix_node **list, const char *prefix, context_node *context) {
    size_t prefix_len = strlen(prefix);

    auto next_list = list;

    while (*next_list) {
        if ((*next_list)->prefix_len < prefix_len || (*next_list)->prefix[0] == '*') {
            list_add(next_list, prefix, context);
            return;
        }
        next_list = &(*next_list)->next;
    }
    list_add(next_list, prefix, context);
}

template <typename List, typename Func>
static void list_foreach(List* list, Func func) {
    while (list) {
        func(list);
        list = list->next;
    }
}

template <typename List, typename Func>
static List* list_find(List* list, Func func) {
    while (list) {
        if (func(list)) {
            return list;
        }
        list = list->next;
    }
    return nullptr;
}

template <typename List>
static void list_free(List** list) {
    while (*list) {
        auto old_list = *list;
        *list = old_list->next;
        delete old_list;
    }
}

void context_node::reset_access() {
    if (!check_access()) {
        unmap();
        no_access_ = true;
    } else {
        no_access_ = false;
    }
}

bool context_node::check_access() {

    ScopedBuffer<char, PATH_MAX> buffer;
    int len = snprintf(buffer.buffer(), PATH_MAX, "%s/%s", kv_dir, context_);

    if (len < 0 || len > PATH_MAX) {
        return false;
    }
    return access(buffer.buffer(), R_OK) == 0;
}

bool context_node::open(bool access_rw) {
    ScopedMutexLock _lock(&mLock);
    if (pa_) {
        return true;
    }

    ScopedBuffer<char, PATH_MAX> buffer;
    int len = snprintf(buffer.buffer(), PATH_MAX, "%s/%s", kv_dir, context_);

    if (len < 0 || len > PATH_MAX) {
        LOGE("context node open failed!");
        return false;
    }

    if (access_rw) {

        pa_ = map_area_rw(TYPE_PA, buffer.buffer());
    } else {
        pa_ = map_prop_area(buffer.buffer());
    }
    return pa_;
}

bool context_node::check_access_and_open() {
    if (!pa_ && !no_access_) {
        if (!check_access() || !open(false)) {
            no_access_ = true;
        }
    }
    return pa_;
}

void context_node::unmap() {
    if (!pa_) {
        return;
    }

    munmap(pa_, pa_->areaSize());
    pa_ = nullptr;
}

static kv_area* get_prop_area_for_name(const char* name) {
    if (!name) {
        return nullptr;
    }
    auto entry = list_find(prefixes, [name](prefix_node* l) {
        return l->prefix[0] == '*' || !strncmp(l->prefix, name, l->prefix_len);
    });
    if (!entry) {
        return nullptr;
    }

    auto cnode = entry->context;
    if (!cnode->pa()) {
        cnode->open(false);
    }
    return cnode->pa();
}


static inline int read_spec_entry(char **entry, char **ptr, int *len)
{
    *entry = NULL;
    char *tmp_buf = NULL;

    while (**ptr == ENTRY_SEPRATOR && **ptr != '\0')
        (*ptr)++;

    tmp_buf = *ptr;
    *len = 0;

    while (**ptr != ENTRY_SEPRATOR && **ptr != '\0') {
        (*ptr)++;
        (*len)++;
    }

    if (*len) {
        *entry = strndup(tmp_buf, *len);
        if (!*entry)
            return -1;
    }

    return 0;
}

static int read_spec_entries(char *line_buf, int num_args, ...)
{
    char **spec_entry, *buf_p;
    int len, rc, items, entry_len = 0;
    va_list ap;

    len = strlen(line_buf);
    if (line_buf[len - 1] == '\n') {
        line_buf[len - 1] = '\0';
    } else {
        len++;
    }

    buf_p = line_buf;
    while (isspace(*buf_p))
        buf_p++;

    /*comment and empty lines. */
    if (*buf_p == '#' || *buf_p == '\0')
        return 0;

    va_start(ap, num_args);

    items = 0;
    while (items < num_args) {
        spec_entry = va_arg(ap, char **);

        if (len - 1 == buf_p - line_buf) {
            va_end(ap);
            return items;
        }
        rc = read_spec_entry(spec_entry, &buf_p, &entry_len);
        if (rc < 0) {
            va_end(ap);
            return rc;
        }
        if (entry_len)
            items++;
    }
    va_end(ap);
    return items;
}

static bool initialize_context() {
    FILE *file = fopen(context_path, "re");

    if (!file) {
        return false;
    }

    char *name_prefix = nullptr;
    char *context = nullptr;

    ScopedBuffer<char, KV_KEY_MAX + KV_VALUE_MAX + 4> line_buf;
    char *line = line_buf.buffer();
    size_t line_len = line_buf.size();

    while (fgets(line, line_len, file)) {
        int items = read_spec_entries(line, 2, &name_prefix, &context);
        if (items <= 0) {
            continue;
        }
        if (items == 1) {
            free(name_prefix);
            continue;
        }

        auto old_context = list_find(
                contexts, [context](context_node *l) {
                    return !strcmp(l->context(), context);
                });
        if (old_context) {
            list_add_after_len(&prefixes, name_prefix, old_context);
        } else {
            list_add(&contexts, context, nullptr);
            list_add_after_len(&prefixes, name_prefix, contexts);
        }
        free(name_prefix);
        free(context);
    }

    fclose(file);
    return true;
}

static void free_and_unmap_contexts() {
    list_free(&prefixes);
    list_free(&contexts);

    if (kv_dir) {
        free(kv_dir);
        kv_dir = nullptr;
    }

    if (context_path) {
        free(context_path);
        context_path = nullptr;
    }

    if (sock_path) {
        free(sock_path);
        sock_path = nullptr;
    }

    if (_sa_) {
        _sa_->clear();
        munmap(_sa_, _sa_->tab_size);
        _sa_ = nullptr;
    }
}

static int ensureDirExist(const char* dir) {
    int ret = -1;
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
                LOGE("ensureDirExist %s failed!", dir);
            }
        }
        else if (ret < 0) {
            LOGE("ensureDirExist %s failed!", dir);
        }
    }
    return ret;
}


const kv_info* find(const char *name)
{
    kv_area* pa = get_prop_area_for_name(name);
    if (!pa) {
        LOGD("\"%s\" -> value not found!", name);
        return nullptr;
    }

    return pa->find(name);
}


static inline uint_least32_t load_const_atomic(const atomic_uint_least32_t* s,
                                               memory_order mo) {
    atomic_uint_least32_t* non_const_s = const_cast<atomic_uint_least32_t*>(s);
    return atomic_load_explicit(non_const_s, mo);
}

unsigned int _serial(const kv_info *pi)
{
    uint32_t serial = load_const_atomic(&pi->serial, memory_order_acquire);
    while (SERIAL_DIRTY(serial)) {
        __futex_wait(const_cast<volatile void *>(
                             reinterpret_cast<const void *>(&pi->serial)),
                     serial, NULL);
        serial = load_const_atomic(&pi->serial, memory_order_acquire);
    }
    return serial;
}

int get_value_by_info(const kv_info *pi, char *name, char *value)
{
    while (true) {
        uint32_t serial = _serial(pi);
        size_t len = SERIAL_VALUE_LEN(serial);
        strtab_info* strinfo = to_strtab_info(pi->str_info);
        if (strinfo)memcpy(value, &strinfo->data[strinfo->nameLen + 1], len + 1);
        atomic_thread_fence(memory_order_acquire);
        if (serial ==
            load_const_atomic(&(pi->serial), memory_order_relaxed)) {
            if (name != 0 && strinfo) {
                strcpy(name, &strinfo->data[0]);
            }
            return len;
        }
    }
}

int get_kv(const char *name, char *value)
{
    size_t nameLen = strlen(name);

    if (nameLen >= KV_KEY_MAX) {
        LOGD("get_value failed for big-key-len: %s", name);
        return -1;
    }

    const kv_info *pi = find(name);

    if (pi != 0) {
        return get_value_by_info(pi, 0, value);
    } else {
        value[0] = 0;
        return 0;
    }
}

int client_set_kv(const char *name, const char *value)
{
    if (name == 0) return -1;
    if (value == 0) value = "*";
    size_t namelen = strlen(name);
    if (namelen >= KV_KEY_MAX) return -1;
    size_t valuelen = strlen(value);
    if (valuelen >= KV_VALUE_MAX) return -1;

    kv_msg msg;
    memset(&msg, 0, sizeof msg);
    msg.cmd = MSG_SET_VALUE;
    msg.key = (char*)malloc(namelen + 1);
    if (msg.key == nullptr) {
        return -1;
    }
    msg.value = (char*)malloc(valuelen + 1);
    if (msg.value == nullptr) {
        return -1;
    }
    strlcpy(msg.key, name, sizeof msg.key);
    msg.key[namelen] = 0;
    strlcpy(msg.value, value, sizeof msg.value);
    msg.value[valuelen] = 0;

    size_t msg_len = sizeof(msg.cmd) + namelen + valuelen + 2;

    const int err = send_stack_msg(&msg, msg_len);
    if (err < 0) {
        return err;
    }

    return 0;
}

int update_value(kv_info *si, const char *value, unsigned int len)
{
    if (len >= KV_VALUE_MAX) {
        return -1;
    }

    uint32_t serial = atomic_load_explicit(&si->serial, memory_order_relaxed);
    serial |= 1;
    atomic_store_explicit(&si->serial, serial, memory_order_relaxed);
    // The memcpy call here also races.  Again pretend it
    // used memory_order_relaxed atomics, and use the analogous
    // counterintuitive fence.
    atomic_thread_fence(memory_order_release);
    strtab_info *strinfo = to_strtab_info(si->str_info);
    if (strinfo) {
        bool hasSetPass = false;
        if (strinfo->valueLen > 0) {
            int stackIndex = strinfo->nameLen + 1;
            if (strinfo->data[stackIndex] == '*' && *value != '*') {
                LOGW("update_value ignore path: %s already *\n",
                     &strinfo->data[0]);
                hasSetPass = true;
            }
        }
        if (!hasSetPass) {
            if (strinfo->valueLen < len) {
                uint_least32_t new_off = 0;
                strtab_info *new_info = nullptr;
                if (allocate_strtab_info(strinfo->nameLen, len, &strinfo->data[0], value,
                                         &new_off)) {
                    new_info = to_strtab_info(new_off);
                } else {
                    len = 0;
                }
                if (new_info != nullptr) {
                    new_info->nameLen = strinfo->nameLen;
                    new_info->valueLen = len;
                    char *data = new_info->data;
                    memcpy(data, &strinfo->data[0], strinfo->nameLen);
                    data += strinfo->nameLen;
                    *data++ = 0;
                    memcpy(data, value, len);
                    data += len;
                    *data = 0;
                }
            } else {
                int stackIndex = strinfo->nameLen + 1;
                memcpy(&strinfo->data[stackIndex], value, len + 1);
            }
        } else {
            len = 1;
        }
    } else {
        len = 0;
    }
    atomic_store_explicit(
            &si->serial,
            (len << SERIAL_VALUE_SHIFT) | ((serial + 1) & 0xffff),
            memory_order_release);
    __futex_wake(&si->serial, INT32_MAX);

    return 0;
}

int add_value(const char *name, unsigned int namelen,
              const char *value, unsigned int valuelen)
{
    if (namelen >= KV_KEY_MAX) {
        return -1;
    }
    if (valuelen >= KV_VALUE_MAX) {
        return -1;
    }
    if (namelen < 1) {
        return -1;
    }

    kv_area* pa = get_prop_area_for_name(name);

    if (!pa) {
        LOGE("add_value failed \"%s\"", name);
        return -1;
    }

    bool ret = pa->add(name, namelen, value, valuelen);
    if (!ret) {
        return -1;
    }

    return 0;
}

int server_set_kv(const char* name, const char* value) {

    if (name == NULL || value == NULL) {
        return -1;
    }
    size_t namelen = strlen(name);
    size_t valuelen = strlen(value);

    if (namelen >= KV_KEY_MAX || valuelen >= KV_VALUE_MAX) {
        LOGE("server_set_kv failed key: %s", name);
        return -1;
    }

    kv_info* pi = (kv_info*) find(name);

    if(pi != 0) {
        update_value(pi, value, valuelen);
    } else {
        int rc = add_value(name, namelen, value, valuelen);
        if (rc < 0) {
            LOGE("server_set_kv failed key: %s valuelen: %zu", name, valuelen);
            return rc;
        }
    }

    return 0;
}

void load_kv(const char* file) {
    LOGD("load_kv: %s", file);
    FILE* propFile = fopen(file, "re");

    if (!propFile) {
        return;
    }

    char* name = nullptr;
    char* value = nullptr;

    ScopedBuffer<char, KV_KEY_MAX + KV_VALUE_MAX + 4> line_buf;
    char *line = line_buf.buffer();
    size_t line_len = line_buf.size();

    while (fgets(line, line_len, propFile)) {
        size_t len = strlen(line);
        bool largeLine = false;
        if (line[len - 1] != '\n' || len > KV_VALUE_MAX) {
            largeLine = true;
        }

        int items = read_spec_entries(line, 2, &name, &value);
        if (items <= 0) {
            continue;
        }

        if (items == 1) {
            free(name);
            continue;
        }

        if (largeLine) {
            LOGE("load_kv from %s, but key-value is too long! set %s ====> *!", file, name);
        }
        server_set_kv(name, largeLine ? "*" : value);

        free(name);
        free(value);
    }

    fclose(propFile);
}

static bool map_strtab(bool access_rw) {
    ScopedBuffer<char, KV_KEY_MAX> buffer;
    int len = snprintf(buffer.buffer(), PATH_MAX, "%s/strtab", kv_dir);
    if (len < 0 || len > PATH_MAX) {
        _sa_ = nullptr;
        return false;
    }

    if (access_rw) {
        _sa_ = map_strtap_rw(BIG_SIZE, buffer.buffer());
    } else {
        _sa_ = map_strtab(buffer.buffer());
    }
    return _sa_;
}

int client_kv_init(const char* dataDir) {
    if (initialized) {
        list_foreach(contexts, [](context_node* l) { l->reset_access(); });
        return 0;
    }

    ScopedBuffer<char, KV_KEY_MAX> buffer;
    snprintf(buffer.buffer(), PATH_MAX, KV_DIRNAME, dataDir);
    struct stat sb;
    if (lstat(buffer.buffer(), &sb) == -1 || S_ISDIR(sb.st_mode)) {
        LOGE("kv_init failed! not found kv_dir");
        return -1;
    }

    kv_dir = strdup(buffer.buffer());

    buffer.buffer()[0] = 0;
    snprintf(buffer.buffer(), PATH_MAX, CONTEXT_FILE, dataDir);

    if (access(buffer.buffer(), R_OK) == 0) {
        LOGE("kv_init failed! not found context_path");
        return -1;
    }
    context_path = strdup(buffer.buffer());

    if (!initialize_context()) {
        free(kv_dir);
        free(context_path);
        kv_dir = nullptr;
        context_path = nullptr;
        return -1;
    }

    if (!map_strtab(false)) {
        free_and_unmap_contexts();
        return -1;
    }

    ScopedBuffer<char, UNIX_PATH_MAX> dirBuffer;
    ScopedBuffer<char, UNIX_PATH_MAX> unixPathBuffer;
    snprintf(dirBuffer.buffer(), UNIX_PATH_MAX, SERVICE_DIR, dataDir);

    if (make_unix_path(dirBuffer.buffer(), unixPathBuffer.buffer(), UNIX_PATH_MAX, SERVICE_PREFIX, SERVICE_PORT) < 0) {
        LOGE("kv_init make socket path failed: %s\n", strerror(errno));
        free_and_unmap_contexts();
        return -1;
    }

    sock_path = strdup(unixPathBuffer.buffer());

    initialized = true;

    LOGD("kv_init success! stack_dir(%s) context_path(%s) sock_path(%s)", kv_dir, context_path, sock_path);
    return 0;
}

int server_kv_init(const char* dataDir) {
    free_and_unmap_contexts();

    ScopedBuffer<char, PATH_MAX> buffer;
    snprintf(buffer.buffer(), PATH_MAX, KV_DIRNAME, dataDir);
    ScopedBuffer<char, PATH_MAX> removeBuffer;
    snprintf(removeBuffer.buffer(), PATH_MAX, "rm -rf %s", buffer.buffer());
    system(removeBuffer.buffer());
    if (ensureDirExist(buffer.buffer()) < 0) {
        LOGE("server_kv_init failed! not found kv_dir");
        return -1;
    }
    kv_dir = strdup(buffer.buffer());

    buffer.buffer()[0] = 0;
    snprintf(buffer.buffer(), PATH_MAX, CONTEXT_FILE, dataDir);

    if (access(buffer.buffer(), R_OK) != 0) {
        LOGE("server_kv_init failed! not found context_path");
        return -1;
    }
    context_path = strdup(buffer.buffer());

    mkdir(kv_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (!initialize_context()) {
        free(kv_dir);
        free(context_path);
        kv_dir = nullptr;
        context_path = nullptr;
        return -1;
    }
    bool open_failed = false;
    list_foreach(contexts, [&open_failed](context_node* l) {
        if (!l->open(true)) {
            open_failed = true;
        }
    });
    if (open_failed || !map_strtab(true)) {
        free_and_unmap_contexts();
        return -1;
    }

    ScopedBuffer<char, UNIX_PATH_MAX> dirBuffer;
    ScopedBuffer<char, UNIX_PATH_MAX> unixPathBuffer;
    snprintf(dirBuffer.buffer(), UNIX_PATH_MAX, SERVICE_DIR, dataDir);

    if (make_unix_path(dirBuffer.buffer(), unixPathBuffer.buffer(), UNIX_PATH_MAX, SERVICE_PREFIX, SERVICE_PORT) < 0) {
        LOGE("server_kv_init make socket path failed: %s\n", strerror(errno));
        free_and_unmap_contexts();
        return -1;
    }

    sock_path = strdup(unixPathBuffer.buffer());
    initialized = true;

    LOGD("server_kv_init success! kv_dir(%s) context_path(%s) sock_path(%s)", kv_dir, context_path, sock_path);
    return 0;
}