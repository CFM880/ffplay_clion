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
set(ffmpeg_prefix "/usr/local/Cellar/ffmpeg/4.1.3_1")
set(ffmpeg_exec_prefix "${ffmpeg_prefix}")
set(libdir "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3_1")
set(FFMPEG_EXEC_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3_1")
set(FFMPEG_LIBDIR "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_INCLUDE_DIRS "${ffmpeg_prefix}/include/")

set(FFMPEG_ENV
        avcodec
        avdevice
        avfilter
        avformat
        avresample
        avutil
        postproc
        swresample
        swscale
        SDL2)

include_directories(
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        header
)


link_directories(
        ${FFMPEG_LIBDIR}
        ${SDL2_LIBDIR}
)



add_executable(ffmpeg_demo ffplay.c)
add_executable(test_ffmpeg main.cpp)
add_executable(test_audio_video testReadInfo.cpp)
add_executable(save_files ext/01RtspSource-SaveFile.cpp)
add_executable(transcode ext/02StreamSource-Transcode.cpp)
add_executable(pcmtoaac ext/03PcmToAac.cpp)
add_executable(video_cat ext/04VideoCat.cpp)

set(SOURCE_FILES
        cmdutils.c,
        ffplay.c,
        main.cpp,
        testReadInfo.cpp,
        ext/01RtspSource-SaveFile.cpp
        ext/02StreamSource-Transcode.cpp
        ext/03PcmToAac.cpp
        ext/04VideoCat.cpp
        )


target_link_libraries(
        ffmpeg_demo
        ${FFMPEG_ENV}
)

target_link_libraries(
        test_ffmpeg
        ${FFMPEG_ENV}
)

target_link_libraries(
        test_audio_video
        ${FFMPEG_ENV}
)

target_link_libraries(
        save_files
        ${FFMPEG_ENV}
)
target_link_libraries(
        transcode
        ${FFMPEG_ENV}
)
target_link_libraries(
        pcmtoaac
        ${FFMPEG_ENV}
)
target_link_libraries(
        video_cat
        ${FFMPEG_ENV}
)
