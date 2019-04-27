### ffplay Mac CLion Debug调试

一直想做一个想做一个Android APP 
1. 在息屏的是后一个能够播放视频的音频，这样就可以在做枯燥无聊重复性的事情的时候又可以学习的东西（例如健身，跑步机上跑步），充分利用网上的视频学习资源
2. 具备自定义导入本地视频目录排序规则，一般的网上视频都是具有目录结构，如何快速的导入视频，并生成播放列表。
目前想到的就只有这两个需求痛点，先完成再说其他的

### JsonMi播放器第一步 学习FFmpeg

#### 前言
既然是一个视频APP，学习视频处理的王者是很有必要的
如果一上来就在Android，首先得学习OpenSE OpenGL，还有各种编译，依赖的条件太多
故直接在Mac平台上学习是极好的。

#### 编译
编译方面不要纠结，直接选择最默认的方式
```$shell
./configure
# make clean 
# make -j8
# make install
```
最后编译后的输出文件路径是/usr/local/Cellar/ffmpeg/4.1.3
在编译Android后，再编译的Mac的，在中间遇到过的一些问题
1. udp结构体不兼容，直接将相关代码注释掉
2. B0问题，B0在android相关代码中有过定义，将相关B0改成b0

可以参考

https://blog.csdn.net/iamcxl369/article/details/79900492

https://www.jianshu.com/p/484db5ec733f

## CMakeList.txt 配置

用CLion建立一个C++项目，在项目根目录下就有一个CMakeList.txt文件

### 相关常量设置

#### 添加SDL2依赖
brew install sdl2 就可以安装sdl2库，在/usr/local/Cellar/sdl2/目录下
将/usr/local/Cellar/sdl2/2.0.9_1/lib/cmake/SDL2/sdl2-config.cmake里的内容
copy到CMakeList.txt里，如下
```cmake
...
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


```

这是对SDL2库相关路径的定义

#### 添加FFmpeg依赖
对于FFmpeg的依赖可以按照类似的写法定义上

如下
```cmake
# FFmpeg
set(ffmpeg_prefix "/usr/local/Cellar/ffmpeg/4.1.3")
set(ffmpeg_exec_prefix "${ffmpeg_prefix}")
set(libdir "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3")
set(FFMPEG_EXEC_PREFIX "/usr/local/Cellar/ffmpeg/4.1.3")
set(FFMPEG_LIBDIR "${ffmpeg_exec_prefix}/lib")
set(FFMPEG_INCLUDE_DIRS "${ffmpeg_prefix}/include/")

```

### 添加依赖头文件夹，库文件文件夹
```cmake
include_directories(
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)


link_directories(
        ${FFMPEG_LIBDIR}
        ${SDL2_LIBDIR}
)
```

### 添加源文件到项目中
将ffmpeg当中的tools/ffplay.c tools/cmdutils.h tools/cmdutils.c config.h拷到工程目录下

将cmdutils.c 标红的三个头文件注释掉
```
// #include "compat/va_copy.h"
// #include "libavutil/libm.h"
// #include "libavformat/network.h"
```
同时在CMakeList.txt设置要编译的源文件
如下
```cmake
set(SOURCE_FILES
        cmdutils.c,
        ffplay.c,
        main.cpp
        )
```
将ffplay.c源文件中的
```
#include "cmdutils.h"
改为
#include "cmdutils.c"
```
C语言当中的include的实际意义，可以简单理解，就是把源文件的编译后内容copy一份到该文件中，但是要防止重复引用，但是我们简单一点就没有加#pragma once，防止重复引用的做法
#
### 可执行程序与文件关联
```cmake
add_executable(ffmpeg_demo ffplay.c)
add_executable(test_ffmpeg main.cpp)
```

### 添加执行文件的依赖库
```cmake
target_link_libraries(
        ffmpeg_demo # 生成的执行文件，与上方add_executable(ffmpeg_demo ffplay.c)一致
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

```
最后build一下，就可以看到两个可运行选项了

## 总结
这次移植，让我了解到，建立CMake项目的基本流程
1. 定义依赖库相关常量，一般包含都文件夹，编译生成的库文件夹
2. 添加库头文件夹，编译生成的库文件夹到工程
3. copy 源码到工程下，添加源码文件到CMakeList.txt中
4. 定义可执行文件，可执行文件与源码关联
5. 为可执行文件添加依赖库

实例demo
