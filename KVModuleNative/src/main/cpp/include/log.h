//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_LOG_H
#define DEMO_LOG_H
#ifdef ENABLE_DEBUG
#define DEBUG
#endif

//PT represents property debug
#define FORCE_TAG "PD-native"

#if defined(FORCE_TAG)
#undef LOG_TAG
#define LOG_TAG FORCE_TAG
#else
#ifndef LOG_TAG
    #define LOG_TAG "storage"
  #endif
#endif
#include <android/log.h>
#ifdef DEBUG
#define LOGD(fmt, args...)  do {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args);} while(0)
#define LOGI(fmt, args...)  do {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args);} while(0)
#else
#define LOGD(fmt, args...) do {} while(0)
#define LOGI(fmt, args...) do {} while(0)
#endif

#define LOGW(fmt, args...)  do {__android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args);} while(0)
#define LOGE(fmt, args...)  do {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args);} while(0)

#endif //DEMO_LOG_H
