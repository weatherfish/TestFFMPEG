cmake_minimum_required(VERSION 3.22)
project(TestFFmpeg)

# 定义一个选项
set(USE_M1 "True" CACHE STRING "Description of My USE_M1")

# 或者根据选项的值来编译不同的源文件
# if(USE_M1 STREQUAL "True")
#     set(FFMPEG_H /opt/ffmpeg/include)
# else()
#     set(FFMPEG_H /opt/ffmpeg/include)
# endif()

# include_directories(${FFMPEG_H})

if(USE_M1  STREQUAL "True")
    # set(SDL3 /usr/local/lib/libSDL3.dylib)

#     set(AVCODEC /opt/ffmpeg/lib/libavcodec.61.6.100.dylib)
#     set(AVDEVICE /opt/ffmpeg/lib/libavdevice.61.2.100.dylib)
#     set(AVFILTER /opt/ffmpeg/lib/libavfilter.10.2.102.dylib)
#     set(AVFORMAT /opt/ffmpeg/lib/libavformat.61.3.104.dylib)
#     set(AVUTIL /opt/ffmpeg/lib/libavutil.59.21.100.dylib)
#     set(SWRSAMPLE /opt/ffmpeg/lib/libswresample.5.2.100.dylib)
#     set(SWSCALE /opt/ffmpeg/lib/libswscale.8.2.100.dylib)
else()
#     set(AVCODEC /opt/ffmpeg/lib/libavcodec.61.6.100.dylib)
#     set(AVDEVICE /opt/ffmpeg/lib/libavdevice.61.2.100.dylib)
#     set(AVFILTER /opt/ffmpeg/lib/libavfilter.10.2.102.dylib)
#     set(AVFORMAT /opt/ffmpeg/lib/libavformat.61.3.104.dylib)
#     set(AVUTIL /opt/ffmpeg/lib/libavutil.59.21.100.dylib)
#     set(SWRSAMPLE /opt/ffmpeg/lib/libswresample.5.2.100.dylib)
#     set(SWSCALE /opt/ffmpeg/lib/libswscale.8.2.100.dylib)
endif()

# link_libraries(${AVCODEC} ${AVDEVICE} ${AVFILTER} ${AVFORMAT} ${AVUTIL} ${SWRSAMPLE} ${SWSCALE} )
# link_libraries(${SDL3})
# include_directories(/usr/local/include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)

pkg_check_modules(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

# add_definitions(-DDEBUG)
# add_definitions(-D__STDC_CONSTANT_MACROS)

# add_subdirectory(glFramework)

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB SOURCE_FILES "*.cpp")
# file(GLOB_RECURSE SOURCE_FILES "*.c")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# target_link_libraries(FFmpegDemo "-framework FFMPEG")
# target_link_libraries(FFmpegDemo)
target_link_libraries(${PROJECT_NAME} PkgConfig::LIBAV ${SDL2_LIBRARIES})
