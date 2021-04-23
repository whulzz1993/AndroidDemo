//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_UTILS_H
#define DEMO_UTILS_H
#include "cdefs.h"
typedef void* thread_id_t;
typedef void* (*thread_func_t)(void*);

bool normalize_path(const char* path, char* const normalized_path);
int createDetachThread(thread_func_t entryFunction, void* userData, thread_id_t* threadId);
int make_unix_path(char *dir, char* outBuffer, size_t  pathlen, const char* prefix, int port_number);

int socket_local_bind(const char* path, int socketType);

int get_module_full_path(pid_t pid, const char* module_name, char *out);

int get_module_full_path_from_addr(pid_t pid, void *addr, char *out);
bool isPathCanWrite(const char* path);

#endif //DEMO_UTILS_H
