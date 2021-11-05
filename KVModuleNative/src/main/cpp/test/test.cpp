//
// Created by admin on 2021/3/22.
//

#include <dlfcn.h>
#include "test.h"
#include "log.h"

namespace Test {

    extern "C" {
        void*  _ZN3art7Runtime9instance_E __attribute__((weak));
    }

    void main() {
        void* xx = nullptr;
        int yy = 3;
        void* zz = &yy;
        //"mov %0, lr \n"
#if defined(__aarch64__)
        asm (
                "add %0, %1, #2\n"
                :[xx]"=r"(xx)
                : "r"(zz)
                : "lr", "x0"
                );

#endif
        LOGD("Test xx: %p", xx);

        void* handle = dlopen("libart.so", RTLD_NOW);
        if (handle == nullptr) {
            LOGE("Test dlopen libart.so failed");
        }
    }

}