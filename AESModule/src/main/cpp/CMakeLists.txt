CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
project(aesmodule LANGUAGES C CXX)

set(AES_SOURCES
        test/aes_test.c
        aes.c
        LibEntry.c)

add_library(aesmodule SHARED ${AES_SOURCES})

target_link_libraries(aesmodule log)