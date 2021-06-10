
//
// Created by admin on 2021/3/31.
//

#include <stdio.h>
#include <jni.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/un.h>
#include <pthread.h>

#define LOG_TAG "rbcore"
#include "log.h"
#include "ScopedLocalEnv.h"
#include "ScopedLocalUtfStr.h"

#include "rbtree.h"
#include "monitor_file.h"

static struct NodeMaster* rbMaster;

static char fileSockPath[UNIX_PATH_MAX];
static bool isServer = false;
static JavaVM* globalVm;

#define INIT_ENV \
        ScopedLocalEnv scopedEnv(globalVm); \
        int status = scopedEnv.initSuccess(); \
        if (!status) { \
            LOGE(" %s getCurrentThread env Failed", __FUNCTION__); \
        } \
        JNIEnv* env = scopedEnv.getEnv();

#ifdef DEBUG

static void nativeTest(JNIEnv* env, jclass clazz) {
    enableLog(true);
    if (!rbMaster) {
        rbMaster = monitor_create(NULL, PAGE_SIZE);
    }
    const char* TEST0[] = {
            "data",
            "data",
            "com.yy",
            "files",
            "yy",
            NULL
    };

    const char* TEST1[] = {
            "data",
            "data",
            "com.yy",
            "cache",
            "yy",
            NULL
    };

    auto insert = [](const char** ptr) {
        const char** begin = ptr;
        while (*ptr) {
            monitor_insert(rbMaster, *ptr);
            ptr++;
        }
    };

    insert(TEST0);
    insert(TEST1);

    Nodefn func = [](struct Node* node) {
        LOGD("node data: %s level: %d", node->data, node->level);
    };

    monitor_foreach(&rbMaster->rb, func);
}

static void printJavaStack() {
    INIT_ENV;
    if (env) {
        jclass throwableClass = env->FindClass("java/lang/Throwable");
        jmethodID defConstructor = env->GetMethodID(throwableClass, "<init>", "()V");
        jobject throwObj = env->NewObject(throwableClass, defConstructor);
        if (throwObj != NULL) {
            jmethodID fillInSTackTraceMethod = env->GetMethodID(throwableClass,
                    "fillInStackTrace", "()Ljava/lang/Throwable;");
            env->CallObjectMethod(throwObj, fillInSTackTraceMethod);
            jmethodID printMethod = env->GetMethodID(throwableClass, "printStackTrace", "()V");
            env->CallVoidMethod(throwObj, printMethod);
        }
    }
}
#endif

static int nativeInitRbModule(JNIEnv* env, jclass clazz, jstring jDataDir, jboolean server) {
    isServer = server;
    return 0;
}

static const char *className = "com/core/rbtree/util/JniBridge";
static JNINativeMethod methods[] = {
        {"NIRM", "(Ljava/lang/String;Z)I", (void*) nativeInitRbModule},
#ifdef DEBUG
        {"test", "()V", (void*)nativeTest},
#endif
};
typedef union {
    JNIEnv* env;
    void* venv;
} UnionJNIEnvToVoid;

static int registerNativeMethods(JNIEnv* env, const char* className,
                                 const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("RegisterNatives unable to find class '%s'", className);
        env->ExceptionClear();
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static int registerNatives(JNIEnv* env)
{
    if (!registerNativeMethods(env, className,
            methods, sizeof(methods) / sizeof(methods[0]))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv* env = NULL;

    LOGD("JNI_OnLoad");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("ERROR: GetEnv failed");
        goto bail;
    }
    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE) {
        LOGE("ERROR: registerNatives failed");
        goto bail;
    }
    globalVm = vm;

    result = JNI_VERSION_1_6;

    bail:
    return result;
}
