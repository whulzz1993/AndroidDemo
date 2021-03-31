//
// Created by Admin on 2020/8/10
//

#include <string>
#include "StringFormat.h"

void StringAppendV(std::string* dst, const char* format, va_list ap) {
    char space[1024];

    va_list backupAp;
    va_copy(backupAp, ap);
    int result = vsnprintf(space, sizeof(space), format, backupAp);
    va_end(backupAp);

    if (result < static_cast<int>(sizeof(space))) {
        if (result >= 0) {
            dst->append(space, result);
            return;
        }

        if (result < 0) {
            // Just an error.
            return;
        }
    }

    int length = result + 1;
    char* buf = new char[length];

    va_copy(backupAp, ap);
    result = vsnprintf(buf, length, format, backupAp);
    va_end(backupAp);

    if (result >= 0 && result < length) {
        dst->append(buf, result);
    }
    delete[] buf;
}

std::string StringFormat(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string result;
    StringAppendV(&result, fmt, ap);
    va_end(ap);
    return result;
}

void StringAppend(std::string* dst, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
}
