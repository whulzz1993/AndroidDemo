# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.
project("core.md5")

#SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
option(ENABLE_DEBUG "Enable debug" ON)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)
set(sources
        base/md5.cpp
        )


if(cmake_build_type_lower STREQUAL "release")
    message(VERBOSE "begin build release")
    set(ENABLE_DEBUG OFF CACHE BOOL "" FORCE)
else()
    message(VERBOSE "begin build debug")
endif()
if(ENABLE_DEBUG)
    add_definitions(-DDEBUG)
endif()
add_library(
        md5core_static
        STATIC
        ${sources}
)

add_library(
        md5core
        SHARED
        ${sources}
)

#target_include_directories(md5core PRIVATE
#        include
#        )
#link_directories(${CMAKE_CURRENT_BINARY_DIR})
#
#set_target_properties( md5core_static PROPERTIES OUTPUT_NAME
#        "md5core_static")


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the key of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

#find_library(
#        log-lib
#        log )
#
## Specifies libraries CMake should link to your target library. You
## can link multiple libraries, such as libraries you define in this
## build script, prebuilt third-party libraries, or system libraries.
#
target_link_libraries(
        md5core
        md5core_static
)