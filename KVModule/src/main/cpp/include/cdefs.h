//
// Created by Admin on 2020/8/9
//

#ifndef DEMO_CDEFS_H
#define DEMO_CDEFS_H
#if !defined(DISALLOW_COPY_AND_ASSIGN)
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;  \
    void operator=(const TypeName&) = delete
#endif
#endif //DEMO_CDEFS_H
