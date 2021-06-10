//
// Created by admin on 2021/6/2.
//

#ifndef DEMO_MONITOR_FILE_H
#define DEMO_MONITOR_FILE_H

#if defined(__cplusplus)
#define __BEGIN_DECLS           extern "C" {
#define __END_DECLS             }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

RB_ENTRY(Node);

struct Node {
    struct Entry next;
    int level;
    dev_t st_dev;
    ino_t parent_ino;
    char* data;
};

RB_HEAD(NodeTree, Node);

struct NodeMaster {
    struct NodeTree rb;
    long size_;
    long used_;
    struct NodeMaster *from;
};

RB_PROTOTYPE(NodeTree, Node, next, my_compare)

__BEGIN_DECLS
#ifdef DEBUG
void enableLog(bool);
#endif
struct NodeMaster*
monitor_create(struct NodeMaster *master, size_t size);

void* monitor_insert(struct NodeMaster* master, const char* data);
typedef void (*Nodefn)(struct Node*);
void monitor_foreach(struct NodeTree* head, Nodefn func);
__END_DECLS
#endif //DEMO_MONITOR_FILE_H
