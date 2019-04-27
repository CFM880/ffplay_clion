cmake_minimum_required(VERSION 3.13)
project(ffmpeg_demo)

set(CMAKE_CXX_STANDARD 14)


# SDL2
set(prefix "/usr/local/Cellar/sdl2/2.0.9_1")
set(exec_prefix "${prefix}")
set(libdir "${exec_prefix}/lib")
set(SDL2_PREFIX "/usr/local/Cellar/sdl2/2.0.9_1")
set(SDL2_EXEC_PREFIX "/usr/local/Cellar/sdl2/2.0.9_1")
set(SDL2_LIBDIR "${exec_prefix}/lib")
set(SDL2_INCLUDE_DIRS "${prefix}/include/SDL2")
set(SDL2_LIBRARIES "-L${SDL2_LIBDIR}  -lSDL2")
string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

# FFmpeg
set(ffmpeg_prefix "/usr/local/Cellar/ffmpeg/4.1.3")
set(ffmpeg_exec_prefix "${ffmpeg_prefix}")
set(libdir "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3")
set(FFMPEG_EXEC_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3")
set(FFMPEG_LIBDIR "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_INCLUDE_DIRS "${ffmpeg_prefix}/include/")


include_directories(
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)


link_directories(
        ${FFMPEG_LIBDIR}
        ${SDL2_LIBDIR}
)


add_executable(ffmpeg_demo ffplay.c)
add_executable(test_ffmpeg main.cpp)

set(SOURCE_FILES
        cmdutils.c,
        ffplay.c,
        main.cpp
        )


target_link_libraries(
        ffmpeg_demo
        avcodec
        avdevice
        avfilter
        avformat
        avresample
        avutil
        postproc
        swresample
        swscale
        SDL2
)

target_link_libraries(
        test_ffmpeg
        avcodec
        avdevice
        avfilter
        avformat
        avresample
        avutil
        postproc
        swresample
        swscale
        SDL2
)