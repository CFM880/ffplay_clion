 avformat_open_input(&inputContext, inputUrl.c_str(), nullptr, nullptr);
 
 avformat_find_stream_info(inputContext, nullptr);
 
 int ret = av_read_frame(inputContext, packet.get());
 
 打开实时流
 
init输出context
 
 while(true) {
 
 获取音视频包
 
 解码成frame (av_read_frame)
 
 编码encoded data成压缩后的图片 (decode_data) 
 
 封装
 
}
 
