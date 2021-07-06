//
// Created by admin on 2021/4/1.
//
#include <unistd.h>
#include <android/log.h>
#include <jni.h>

//abort
#include <stdlib.h>
#include <string.h>

//abort
#define LOG_TAG "runtimeFake"
#define DEBUG
#ifdef DEBUG
#define ALOGD(fmt, args...)  do {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args);} while(0)
#define ALOGI(fmt, args...)  do {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args);} while(0)
#else
#define ALOGD(fmt, args...)  do {} while(0)
#define ALOGI(fmt, args...)  do {} while(0)
#endif
#define ALOGE(fmt, args...)  do {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args);} while(0)

static JavaVM* globalVm;

template<typename T>
int findOffset(void *start, int regionStart, int regionEnd, T value) {

    if (NULL == start || regionEnd <= 0 || regionStart < 0) {
        return -1;
    }
    char *c_start = (char *) start;

    for (int i = regionStart; i < regionEnd; i += 4) {
        T *current_value = (T *) (c_start + i);
        if (value == *current_value) {
            ALOGD("found offset: %d", i);
            return i;
        }
    }
    return -2;
}

void fakePrivateApiCheck(JNIEnv* env, jclass clazz, int targetSdkVersion)
{
    void* runtime = *((void**)globalVm + 1);
    const int MAX = 2000;
    int offsetOfVmExt = findOffset(runtime, 0, MAX, (long)globalVm);
    ALOGD("offsetOfVmExt: %d", offsetOfVmExt);

    if (offsetOfVmExt < 0) {
        return;
    }

    int targetSdkVersionOffset = findOffset(runtime, offsetOfVmExt, MAX, targetSdkVersion);
    ALOGD("target: %d", targetSdkVersionOffset);

    if (targetSdkVersionOffset < 0) {
        return;
    }

    int32_t* targetSdkVersionAddr = (int32_t*)((char*)runtime + targetSdkVersionOffset);
#ifdef DEBUG
    for (int index = 0;index < 50;index ++) {
        ALOGD("fakePrivateApiCheck index[%d]%x", index, targetSdkVersionAddr[index]);
    }
#endif
#if defined(__LP64__)
    if (targetSdkVersionAddr[23] == 3 || targetSdkVersionAddr[23] == 2) {
        targetSdkVersionAddr[23] = 0;
        ALOGD("fakePrivateApiCheck: find hidden_policy_ targetSdkVersionAddr[15], 64bit runtime");
    }
#else
    if (targetSdkVersionAddr[11] == 3 || targetSdkVersionAddr[11] == 2) {
        targetSdkVersionAddr[11] = 0;
        ALOGD("fakePrivateApiCheck: find hidden_policy_ targetSdkVersionAddr[11], 32bit runtime");
    }
#endif
}

//static void test_remove(JNIEnv* env, jclass clazz, jstring jpath) {
//    const char* cpath = env->GetStringUTFChars(jpath, 0);
//    remove(cpath);
//    env->ReleaseStringUTFChars(jpath, cpath);
//}

static const char *className = "core/module/runtimefake/RuntimeFake";
static JNINativeMethod methods[] = {
        {"fake", "(I)V", (void*) fakePrivateApiCheck},
//        {"testRemove", "(Ljava/lang/String;)V", (void*) test_remove},
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
        ALOGE("RegisterNatives unable to find class '%s'", className);
        env->ExceptionClear();
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
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

    ALOGD("JNI_OnLoad");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed");
        goto bail;
    }
    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE) {
        ALOGE("ERROR: registerNatives failed");
        goto bail;
    }
    globalVm = vm;
    result = JNI_VERSION_1_6;

    bail:
    return result;
}
