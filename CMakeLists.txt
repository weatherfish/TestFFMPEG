cmake_minimum_required(VERSION 3.12)
project(TestFFmpeg)

# 定义一个选项
set(USE_M1 "True" CACHE STRING "Description of My USE_M1")

# 或者根据选项的值来编译不同的源文件
if(USE_M1 STREQUAL "True")
    set(FFMPEG_H /opt/ffmpeg/include)
else()
    set(FFMPEG_H /opt/ffmpeg/include)
endif()

include_directories(${FFMPEG_H})

if(USE_M1  STREQUAL "True")
    # set(FFMPEG_LINK /opt/ffmpeg/lib/libavcodec.61.6.100.dylib)
    # link_directories(/opt/homebrew/Cellar/ffmpeg/7.0.1/lib)
    link_directories(/opt/ffmpeg/lib/)
else()
    # set(FFMPEG_LINK /opt/ffmpeg/lib/libavcodec.61.6.100.dylib)
    link_directories(/opt/ffmpeg/lib/)
    # link_directories(/opt/homebrew/Cellar/ffmpeg/7.0.1/lib)
endif()

# link_libraries(${FFMPEG_LINK})

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DDEBUG)
add_definitions(-D__STDC_CONSTANT_MACROS)

# add_subdirectory(glFramework)

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB SOURCE_FILES "*.cpp")

add_executable(FFmpegDemo ${SOURCE_FILES})

# target_link_libraries(FFmpegDemo "-framework FFMPEG")
target_link_libraries(FFmpegDemo)
