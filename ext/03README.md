查看设备

```
chengfangmingdeMacBook-Pro:~ chengfangming$ ffmpeg -f avfoundation -list_devices true -i ""
ffmpeg version 4.1.3 Copyright (c) 2000-2019 the FFmpeg developers
  built with Apple LLVM version 10.0.1 (clang-1001.0.46.4)
  configuration: --prefix=/usr/local/Cellar/ffmpeg/4.1.3_1 --enable-shared --enable-pthreads --enable-version3 --enable-hardcoded-tables --enable-avresample --cc=clang --host-cflags='-I/Library/Java/JavaVirtualMachines/adoptopenjdk-11.0.2.jdk/Contents/Home/include -I/Library/Java/JavaVirtualMachines/adoptopenjdk-11.0.2.jdk/Contents/Home/include/darwin' --host-ldflags= --enable-ffplay --enable-gnutls --enable-gpl --enable-libaom --enable-libbluray --enable-libmp3lame --enable-libopus --enable-librubberband --enable-libsnappy --enable-libtesseract --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 --enable-libxvid --enable-lzma --enable-libfontconfig --enable-libfreetype --enable-frei0r --enable-libass --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libopenjpeg --enable-librtmp --enable-libspeex --enable-videotoolbox --disable-libjack --disable-indev=jack --enable-libaom --enable-libsoxr
  libavutil      56. 22.100 / 56. 22.100
  libavcodec     58. 35.100 / 58. 35.100
  libavformat    58. 20.100 / 58. 20.100
  libavdevice    58.  5.100 / 58.  5.100
  libavfilter     7. 40.101 /  7. 40.101
  libavresample   4.  0.  0 /  4.  0.  0
  libswscale      5.  3.100 /  5.  3.100
  libswresample   3.  3.100 /  3.  3.100
  libpostproc    55.  3.100 / 55.  3.100
[AVFoundation input device @ 0x7fcf1f474380] AVFoundation video devices:
[AVFoundation input device @ 0x7fcf1f474380] [0] FaceTime HD Camera
[AVFoundation input device @ 0x7fcf1f474380] [1] Capture screen 0
[AVFoundation input device @ 0x7fcf1f474380] [2] Capture screen 1
[AVFoundation input device @ 0x7fcf1f474380] AVFoundation audio devices:
[AVFoundation input device @ 0x7fcf1f474380] [0] Built-in Microphone
[AVFoundation input device @ 0x7fcf1f474380] [1] WH-1000XM3
```



apple使用的是AVFoundation框架
使用的时候就是按照https://www.ffmpeg.org/ffmpeg-devices.html#avfoundation来使用
采集到是最原始的格式pcm或者y

AVDictionary就是传递参数的载体