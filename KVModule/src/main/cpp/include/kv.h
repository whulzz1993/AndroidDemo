//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_KV_H
#define DEMO_KV_H
#include <unistd.h>
#include "cdefs.h"
typedef enum {
    TYPE_PA = 0,
    TYPE_SA = 1,
    TYPE_BIG = 2,
} TYPE_CONTEXT;

typedef enum {
    TYPE_MATCH_ALL = 0,
    TYPE_MATCH_PREFIX = 1,
} TYPE_MATCH;

typedef struct
{
    unsigned cmd;
    char* key;
    char* value;
} kv_msg;
#define SERVICE_DIR "%s/files/kv"
#define KV_DIRNAME "%s/files/kv/data"
#define SERVICE_PREFIX "server"
#define SERVICE_PORT 16666
#define CONTEXT_FILE "%s/files/kv/init.kv"
#define MSG_SET_VALUE 1
#define KV_KEY_MAX 256
#define KV_VALUE_MAX 256
#define ENTRY_SEPRATOR '^'
#define PA_SIZE (128 * 1024)
#define SA_SIZE (1024 * 1024)
#define BIG_SIZE (4 * 1024 * 1024)
//PROP
#define PROP_AREA_MAGIC 0x504F5250
//000001
#define PROP_AREA_VERSION 0x31303030
#define SERIAL_VALUE_SHIFT 16
#ifndef SERIAL_VALUE_LEN
#define SERIAL_VALUE_LEN(serial) ((serial) >> SERIAL_VALUE_SHIFT)
#endif
#ifndef SERIAL_DIRTY
#define SERIAL_DIRTY(serial) ((serial) & 1)
#endif

#define BYTE_ALIGN(value, alignment) \
  (((value) + (alignment) - 1) & ~((alignment) - 1))



int server_kv_init(const char* dataDir);
void load_kv(const char* path);
int server_set_kv(const char* key, const char* value);

int client_kv_init(const char* dataDir);
int client_set_kv(const char* key, const char* value);

int get_kv(const char *key, char *value);


#endif //DEMO_KV_H
