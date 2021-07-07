//
// Created by admin on 2021/7/7.
//
extern int aes_test();

static void __attribute__((constructor)) init() {
    aes_test();
}