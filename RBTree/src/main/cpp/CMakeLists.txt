# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("core.rbtree")
option(ENABLE_DEBUG "Enable debug" ON)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

if(cmake_build_type_lower STREQUAL "release")
    message(VERBOSE "begin build release")
    set(ENABLE_DEBUG OFF CACHE BOOL "" FORCE)
else()
    message(VERBOSE "begin build debug")
    set(ENABLE_DEBUG ON CACHE BOOL "" FORCE)
endif()

if(ENABLE_DEBUG)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
endif()

#begin static rbtree
add_library(rbtree STATIC librbtree/monitor_file.c librbtree/xmalloc.c)
set(RBTREE_DEFINITIONS)
if(ENABLE_DEBUG)
    set(RBTREE_DEFINITIONS PRIVATE -DDEBUG)
    target_compile_definitions(rbtree ${RBTREE_DEFINITIONS})
endif()
target_include_directories(rbtree PRIVATE librbtree/include/ include/util/)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

#begin shared rbcore
set(RBCORE_LIBRARIES)
list(APPEND RBCORE_LIBRARIES PRIVATE -lrbtree)

set(sources
        JNICore.cpp
        librbtree/monitor_file.c
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the key of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

add_library(
        rbcore
        SHARED
        ${sources}
)
add_dependencies(rbcore rbtree)
set(RBCORE_DEFINITIONS)
if(ENABLE_DEBUG)
    set(RBCORE_DEFINITIONS PRIVATE -DDEBUG)
    target_compile_definitions(rbcore ${RBCORE_DEFINITIONS})
endif()

target_include_directories(rbcore PUBLIC
        include/util
        librbtree/include/
        )

find_library(
        log-lib
        log )

list(APPEND RBCORE_LIBRARIES PRIVATE ${log-lib})

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries(
        rbcore
        ${RBCORE_LIBRARIES}
)