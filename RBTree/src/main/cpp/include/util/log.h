//
// Created by admin on 2021/6/1.
//

#ifndef DEMO_LOG_H
#define DEMO_LOG_H
#ifdef DEBUG
#include <android/log.h>
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#else
#define  LOGE(...) do {} while(0)
#define  LOGW(...) do {} while(0)
#define  LOGD(...) do {} while(0)
#define  LOGI(...) do {} while(0)
#define  LOGV(...) do {} while(0)
#endif
#endif //DEMO_LOG_H
