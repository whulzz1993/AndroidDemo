//
// Created by admin on 2021/6/2.
//
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>

#include "rbtree.h"
#include "monitor_file.h"
#include "xmalloc.h"

#define LOG_TAG "monitor_file"
#include "log.h"

#ifdef DEBUG
static bool _sEnableDebug = false;
void enableLog(bool debug) {
    static bool inited = false;
    if (!inited) {
        _sEnableDebug = debug;
        inited = true;
    }
}
#else
#define
#endif

static int
my_compare(struct Node *a, struct Node *b)
{
    int diff = a->level - b->level;
    if (diff == 0) {
        return strcmp(a->data, b->data);
    } else {
        return diff;
    }
}

RB_GENERATE(NodeTree, Node, next, my_compare)

static struct Node*
make_node(struct NodeMaster* master, struct NodeTree* head, int level, const char* data)
{
    struct Node *tmp = NULL, *tmp2 = NULL;

    if (master->from == NULL) {
        tmp = xmalloc(sizeof(struct Node));
    }

    tmp->data = data;
    tmp->level = level;

    tmp2 = RB_INSERT(NodeTree, head, tmp);
    if (tmp2 != NULL) {
#ifdef DEBUG
        if (_sEnableDebug)
#endif
        LOGE("make_entry(%p): double address (%p)",
             master, tmp2);
    }

    return (tmp);
}

struct NodeMaster*
monitor_create(struct NodeMaster *master, size_t size)
{
    struct NodeMaster *nm;

    if (master == NULL) {
        nm = xmalloc(sizeof(struct NodeMaster));
    }

    nm->from = master;
    nm->size_ = size;
    nm->used_ = sizeof(struct NodeMaster);

    RB_INIT(&nm->rb);

    make_node(nm, &nm->rb, 0, "\0");

    return (nm);
}

void* monitor_insert(struct NodeMaster* master, const char* data) {
    struct Node *node = NULL, *tmp = NULL;

    size_t allocLen = data ? strlen(data) : 0;
    if (allocLen == 0) {
#ifdef DEBUG
        if (_sEnableDebug)
#endif
        LOGE("%s try to allocate 0 space", __FUNCTION__);
        return NULL;
    } else if ((allocLen + master->used_) >= master->size_) {
#ifdef DEBUG
        if (_sEnableDebug)
#endif
        LOGE("%s alloc failed: size too big", __FUNCTION__);
        return NULL;
    }

    RB_FOREACH(node, NodeTree, &master->rb) {
        if (node->level == level && strncmp(node->data, data, allocLen) == 0) {
            break;
        }
    }

    if (node) {
#ifdef DEBUG
        if (_sEnableDebug)
#endif
        LOGD("%s no need to malloc %s", __FUNCTION__,data);
        return node;
    }

    tmp = make_node(master, &master->rb, level, data);

    if (tmp == NULL) {
#ifdef DEBUG
        if (_sEnableDebug)
#endif
        LOGE("%s make node failed", __FUNCTION__);
        return NULL;
    }
    tmp->data = strdup(data);
    tmp->level = level;
    return tmp;
}

void monitor_foreach(struct NodeTree* head, Nodefn func) {
    struct Node* node = NULL;
    RB_FOREACH(node, NodeTree, head) {
        func(node);
    }
}

