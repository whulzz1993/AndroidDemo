//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_STRINGFORMAT_H
#define DEMO_STRINGFORMAT_H
#include "cdefs.h"
#include <string>
std::string StringFormat(const char* fmt, ...);
void StringAppend(std::string* dst, const char* fmt, ...);
void StringAppendV(std::string* dst, const char* format, va_list ap);

#endif //DEMO_STRINGFORMAT_H
