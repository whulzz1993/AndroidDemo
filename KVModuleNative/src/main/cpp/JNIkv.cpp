
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
#include "KVSocketServer.h"

#include "kv.h"
#include "ScopedLocalUtfStr.h"

#define LOG_TAG "StackManager"

#if defined(ENABLE_DEBUG)
#include "test.h"
#endif
#include "log.h"
#include "ScopedBuffer.h"
#include "utils.h"
#include "ScopedLocalEnv.h"
#include "timer.h"

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

static int nativeInit(JNIEnv* env, jclass clazz, jstring jDataDir, jboolean server) {
    isServer = server;
    ScopedLocalUtfStr scopedStr(env, jDataDir);
    if (server) {
        if (server_kv_init(scopedStr.c_str()) != 0) {
            LOGE("server_kv_init failed!");
            return -1;
        }
        ScopedBuffer<char, PATH_MAX> buffer;
        snprintf(buffer.buffer(), PATH_MAX, SERVICE_DIR "/init.kv", scopedStr.c_str());
        load_kv(buffer.buffer());
        KVSocketServer& kvServer = KVSocketServer::defaultKVServer();
        int status = kvServer.init(fileSockPath, UNIX_PATH_MAX, scopedStr.c_str());
        if (status != 1 || !kvServer.start()) {
            LOGE("KVServer start failed!\n");
            return -1;
        } else {
            LOGD("KVServer start success! listen %s\n", fileSockPath);
            return 0;
        }
    } else {
        return client_kv_init(scopedStr.c_str());
    }
}

static jint nativeSetKV(JNIEnv* env, jclass clazz, jstring jkey, jstring jvalue) {
    ScopedLocalUtfStr scopedKey(env, jkey);
    const char* key = scopedKey.c_str();
    if (key == NULL) {
        return -1;
    }
    if (scopedKey.size() >= KV_KEY_MAX) {
        return -1;
    }

    ScopedLocalUtfStr scopedValue(env, jvalue);
    if (scopedValue.c_str() == NULL || scopedValue.size() >= KV_VALUE_MAX) {
        return -1;
    }

    if (isServer) {
        return server_set_kv(key, scopedValue.c_str());
    } else {
        return client_set_kv(key, scopedValue.c_str());
    }
}

static jstring nativeGetKV(JNIEnv* env, jclass clazz, jstring jkey) {
    ScopedLocalUtfStr scopedKey(env, jkey);
    const char* key = scopedKey.c_str();
#ifdef DEBUG
    Timer timer(__FUNCTION__);
    if (key) timer.append("getKV %s", key);
    timer.markBegin();
#endif
    if (key == NULL) {
        return NULL;
    }
    if (scopedKey.size() >= KV_KEY_MAX) {
        return NULL;
    }

    ScopedBuffer<char, KV_VALUE_MAX> valueBuffer;
    int len = get_kv(key, valueBuffer.buffer());
    if (len <= 0) {
        return NULL;
    }

    return env->NewStringUTF(valueBuffer.buffer());
}

static const char *className = "core/module/kv/KVUtils";
static JNINativeMethod methods[] = {
        {"NKVI", "(Ljava/lang/String;Z)I", (void*) nativeInit},
        {"NSKV", "(Ljava/lang/String;Ljava/lang/String;)I", (void*)nativeSetKV},
        {"NGKV", "(Ljava/lang/String;)Ljava/lang/String;", (void*)nativeGetKV},
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
    std::string tmp("whulzz");
    LOGD("tmp: %s", tmp.c_str());

#if defined(ENABLE_DEBUG)
    Test::main();
#endif

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
