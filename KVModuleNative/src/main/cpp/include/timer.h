//
// Created by Admin on 2020/8/9
//

#ifndef DEMO__TIMER_H
#define DEMO__TIMER_H

#ifndef LOG_TAG
#define LOG_TAG "Timer"
#endif
#include "log.h"
#include "StringFormat.h"

uint64_t gettime_ns();

class Timer {
public:
    Timer(const char* func) : t0(gettime_ns()), function(func) {
    }
    ~Timer() {
        LOGD("%s %s took %.2fms.\n", function, str.c_str(), duration());
    }
    void append(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        StringAppendV(&str, format, ap);
        va_end(ap);
    }
    void markBegin() {
        LOGD("%s begin %s\n", function, str.c_str());
    }

    double duration() {
        return static_cast<double>(gettime_ns() - t0) / 1000000.0;
    }

private:
    uint64_t t0;
    const char* function;
    std::string str;
};
#endif //DEMO__TIMER_H
