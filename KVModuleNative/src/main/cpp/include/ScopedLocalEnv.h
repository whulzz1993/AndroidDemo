//
// Created by Admin on 2020/8/10
//

#ifndef DEMO_SCOPEDLOCALENV_H
#define DEMO_SCOPEDLOCALENV_H

#include <unistd.h>
#include <jni.h>
class ScopedLocalEnv {
public:
    ScopedLocalEnv(JavaVM* javaVM) : jvm(javaVM), env(nullptr), initOk(false), attached(false) {
        if (jvm) {
            int status = jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
            if (status != JNI_OK) {
                status = jvm->AttachCurrentThread(&env, NULL);
                if (status < 0) {
                    return;
                } else {
                    attached = true;
                }
            }
            initOk = true;
        }
    }

    ~ScopedLocalEnv() {
        if (attached) {
            jvm->DetachCurrentThread();
        }
    }

    bool initSuccess() const {return initOk;}
    JNIEnv* getEnv() const {
        if (initOk) {
            return env;
        }
        return nullptr;
    }
private:
    JavaVM* jvm;
    JNIEnv* env;
    bool initOk;
    bool attached;
};
#endif //DEMO_SCOPEDLOCALENV_H
