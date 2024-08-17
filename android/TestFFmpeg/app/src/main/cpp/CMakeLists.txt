cmake_minimum_required(VERSION 3.22.1)

project("ffmpeg")

find_library(log-lib log)
find_library(android-lib android)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp)

set(JNI_LIB_DIR ${CMAKE_SOURCE_DIR}/jniLibs)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
    ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavutil.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libswscale.so)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavcodec.so)

add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavdevice.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavfilter.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
        ${JNI_LIB_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavformat.so)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${CMAKE_PROJECT_NAME}
        avutil
        swresample
        swscale
        avcodec
        avformat
        avfilter
        avdevice
        ${log-lib}
        ${android-lib}
        android
        log)