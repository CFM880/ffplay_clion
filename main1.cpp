//
// Created by 程方明 on 2019-04-27.
//
#include <iostream>
// 1.引用头文件
extern "C" {
#include<libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixdesc.h>

}
using namespace std;
int main() {
//    // 2.注册协议，格式与编解码器
//    char *path = "~/Downloads/yyy.mkv";
//    avformat_network_init();
//    av_register_all();
//    AVFormatContext *formatCtx = avformat_alloc_context();
//    // 3. 打开媒体文件，并设置超时回调
//    AVIOInterruptCB int_cb = {interrupt_callback, (__bridge void *)(self)};
//    formatCtx->interrupt_callback = int_cb;
//    avformat_open_input(formatCtx, path, NULL, NULL);
//    avformat_find_stream_info(formatCtx,NULL);
//    int videoStreamIndex = -1;
//    int audioStreamIndex = -1;
//    for (int i = 0; i < formatCtx->nb_streams; ++i) {
//        AVStream *stream = formatCtx->streams[i];
//        if (AVMEDIA_TYPE_VIDEO == stream->codec->codec_type) {
//            videoStreamIndex = i;
//        } else {
//            audioStreamIndex = i;
//        }
//    }
//    // 4. 寻找各个流，并打开对应的解码器
//    // 音频
//    AVStream *audioStream = formatCtx->streams[audioStreamIndex];
//    AVCodecContext *audioCodecCtx = audioStream->codec;
//    AVCodec *codec = avcodec_find_decoder(audioCodecCtx->codec_id);
//    if (!codec) {
//        // 找不到音频的音频解码器
//    }
//    int openCodeError = 0;
//    if ((openCodeError = avcodec_open2(audioCodecCtx, codec, NULL) < 0)){
//        // 打开音频解码器失败
//    }
//    // 视频
//    AVStream *videoStream = formatCtx->streams[videoStreamIndex];
//    AVCodecContext *videoCodecCtx = videoStream->codec;
//    AVCodec *videoCodec = avcodec_find_decoder(audioCodecCtx->codec_id);
//    if (!videoCodec) {
//        // 找不到视频的音频解码器
//    }
//    openCodeError = 0;
//    if ((openCodeError = avcodec_open2(videoCodecCtx, videoCodec, NULL) < 0)){
//        // 打开视频解码器失败
//    }
//    // 5.初始化解码后数据的结构体
//    // 5.1 分配出解码之后数据所存放的内存空间，以及进行格式转换需要的用到的对象
//    SwrContext *swrContext = NULL;
//    if (audioCodecCtx->sample_fmt != AV_SAMPLE_FMT_S16){
//        // 如果不是我们需要的格式
//        swrContext = swr_alloc_set_opts(NULL,
//                outputChannel, AV_SAMPLE_FMT_S16, outSampleTate,
//                in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);
//        if (!swrContext || swr_init(swrContext)){
//            if (swrContext){
//                swr_free(&swrContext);
//            }
//        }
//
//    }
//    AVFrame *audioFrame = av_frame_alloc();
//
//    // 5.2 构建视频的格式转换对象以及视频解码后数据存放的对象
//    AVPicture picture;
//    bool pictureValid = avpicture_alloc(&picture, AV_PIX_FMT_YUV420P,
//            videoCodecCtx->width,
//            videoCodecCtx->height) == 0;
//
//    if (!pictureValid){
//        return -1;
//    }
//    SwsContext *swsContext = NULL;
//    swrContext = sws_getCachedContext(swrContext,
//            videoCodecCtx->width,
//            videoCodecCtx->height,
//            videoCodecCtx->pix_fmt,
//            videoCodecCtx->width,
//            videoCodecCtx->height,
//            AV_PIX_FMT_YUV420P,
//            SWS_FAST_BILINEAR,
//            NULL, NULL, NULL);
//    AVFrame *videoFrame = av_frame_alloc();
//
//    // 6.读取内并解码
//    AVPacket packet;
//    int gotFrame = 0;
//    while (true) {
//        if (av_read_frame(formatCtx, &packet)){
//            // end of file
//            break;
//        }
//        int packetStreamIndex = packet.stream_index;
//        if (packetStreamIndex == videoStreamIndex){
//            int len = avcodec_decode_video2(videoCodecCtx, videoFrame, &gotFrame, &packet);
//            if (len < 0) {
//                break;
//            }
//            if (gotFrame){
//                slef->handleVideoFrame();
//            }
//        } else if (packetStreamIndex == audioStreamIndex) {
//            int len = avcodec_decode_audio4(audioCodecCtx, audioFrame, &gotFrame, &packet);
//            if (len < 0) {
//                break;
//            }
//            if (gotFrame){
//                slef->handleAudioFrame();
//            }
//        }
//    }
//    // 7. 处理解码后的裸数据
//    void *audioData;
//    int numFrames;
//    int channels = 2;
//    if (swrContext) {
//        int bufSize = av_samples_get_buffer_size(NULL, channels,
//                (int) (audioFrame->nb_samples * channels),
//                AV_SAMPLE_FMT_S16P, 1);
//
//        if (!_swrBuffer || _swrBufferSize < bufSize) {
//            swrBufferSize = bufSize;
//            swrBuffer = realloc(_swr)
//        }
//    }
//    // 关闭所有资源
//
//
//    return 0;
}
