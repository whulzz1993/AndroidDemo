//
// Created by admin on 2021/7/7.
//

#ifndef DEMO_LOG_H
#define DEMO_LOG_H

#include <android/log.h>

#define LOG_TAG "AES_TEST"
#define DEBUG
#ifdef DEBUG
#define ALOGD(fmt, args...)  do {__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args);} while(0)
#define ALOGI(fmt, args...)  do {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args);} while(0)
#else
#define ALOGD(fmt, args...)  do {} while(0)
#define ALOGI(fmt, args...)  do {} while(0)
#endif
#define ALOGE(fmt, args...)  do {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args);} while(0)
#endif //DEMO_LOG_H
