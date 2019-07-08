ffmpeg -i ttt.flv -vcodec copy -an -f flv  ttt1.flv 
移除音频

主要对需要数据包处理，裁剪的时候需要，将去掉的部分的一段包的pts校正