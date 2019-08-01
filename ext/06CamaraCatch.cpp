//
// Created by 程方明 on 2019-07-28.
//

#include <string>
#include <memory>
#include <thread>
#include <iostream>
#include <pch.h>

using namespace std;

AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext;

int64_t lastReadPacketTime;


static int interrupt_cb(void *ctx) {
    int timeout = 60;
    if (av_gettime() - lastReadPacketTime > timeout * 1000 * 1000) {
        return -1;
    }
    return 0;
}

void init() {
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    avdevice_register_all();
    av_log_set_level(AV_LOG_WARNING);
}


int openInput(string inputUrl) {
    inputContext = avformat_alloc_context();
    lastReadPacketTime = av_gettime();
    inputContext->interrupt_callback.callback = interrupt_cb;

    AVInputFormat *ifmt = av_find_input_format("avfoundation");
    AVDictionary *formt_opts = nullptr;
    av_dict_set_int(&formt_opts, "framerate", 30, 0);
    av_dict_set(&formt_opts, "video_size", "1280x720", 0);
    av_dict_set(&formt_opts, "pixel_format", "nv12", 0);
    int ret = avformat_open_input(&inputContext, inputUrl.c_str(), ifmt, &formt_opts);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
        return  ret;
    }
    ret = avformat_find_stream_info(inputContext, nullptr);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
    }
    else
    {
        av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n",inputUrl.c_str());
    }
    return ret;
}

int openOutput(string outputUrl, AVCodecContext *encodeCodec) {
    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "flv", outputUrl.c_str());
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open output context failed");
        goto Error;
    }
    ret = avio_open2(&outputContext->pb, outputUrl.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }
    for (int i = 0; i < inputContext->nb_streams; i++) {
        if (inputContext->streams[i]->codec->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            continue;
        }
        AVStream *stream = avformat_new_stream(outputContext, encodeCodec->codec);
        ret = avcodec_copy_context(stream->codec, encodeCodec);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "copy codec context failed");
            goto Error;
        }
    }
    ret = avformat_write_header(outputContext, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "format header write failed");
        goto Error;
    }
    av_log(NULL, AV_LOG_ERROR, "open output file %s success\n", outputUrl.c_str());
    return ret;
    Error:
    if (outputContext) {
        for (int i = 0; i < outputContext->nb_streams; ++i) {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }
    return ret;
}

int initDecodeContext(AVStream *inputStream) {
    auto codecId = inputStream->codec->codec_id;
    auto codec = avcodec_find_decoder(codecId);
    if (!codec) {
        return -1;
    }
    int ret = avcodec_open2(inputStream->codec, codec, nullptr);
    return ret;
}

int initEncodeContext(AVStream *inputStream, AVCodecContext **encodeContext) {
    AVCodec *pictureCodec;
    pictureCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    (*encodeContext) = avcodec_alloc_context3(pictureCodec);

    (*encodeContext)->codec_id = pictureCodec->id;
    (*encodeContext)->time_base.num = inputStream->time_base.num;
    (*encodeContext)->time_base.den = inputStream->time_base.den;
    (*encodeContext)->pix_fmt = AV_PIX_FMT_NV12;
    (*encodeContext)->width = inputStream->codecpar->width;
    (*encodeContext)->height = inputStream->codecpar->height;
    int ret = avcodec_open2(*encodeContext, pictureCodec, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open encode failed");
    }

}


shared_ptr<AVPacket> readPacketFromSources() {
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    lastReadPacketTime = av_gettime();
    int ret = av_read_frame(inputContext, packet.get());
    if (ret >= 0) {
        return packet;
    } else {
        return nullptr;
    }
}


bool decode(AVStream* inputStream,AVPacket* packet, AVFrame *frame)
{
    int gotFrame = 0;
    auto hr = avcodec_decode_video2(inputStream->codec, frame, &gotFrame, packet);
    if (hr >= 0 && gotFrame != 0)
    {
        return true;
    }
    return false;
}
shared_ptr<AVPacket> encode(AVCodecContext *encodeContext, AVFrame *frame) {
    int gotOutput = 0;
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    packet->data = NULL;
    packet->size = 0;
    int ret = avcodec_encode_video2(encodeContext, packet.get(), frame, &gotOutput);
    if (ret >= 0 && gotOutput) {
        return packet;
    } else {
        return nullptr;
    }
}

int writePacket(shared_ptr<AVPacket> packet) {
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    return av_interleaved_write_frame(outputContext, packet.get());
}

void closeInput(){
    if (inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}

void closeOutput() {
    if(outputContext != nullptr)
    {
        int ret = av_write_trailer(outputContext);
        for(int i = 0 ; i < outputContext->nb_streams; i++)
        {
            AVCodecContext *codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&outputContext);
    }
}
int main() {
    init();
    int ret = openInput("0");
    if (ret < 0) {
//        goto Error;
    }
    if (ret < 0) {
//        goto Error;
    }

    AVCodecContext *encodeContext = nullptr;
    int videoIndex;
    for (int i = 0; i < inputContext->nb_streams; i++) {
        if (inputContext->streams[i]->codec->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }


    initDecodeContext(inputContext->streams[videoIndex]);
    initEncodeContext(inputContext->streams[videoIndex], &encodeContext);
    AVFrame *videoFrame = av_frame_alloc();
    ret = openOutput("xxxxxxx", encodeContext);
    int count = 0;
    while (true) {
        auto packet = readPacketFromSources();
        if (packet && packet->stream_index == videoIndex) {
            if(decode(inputContext->streams[0], packet.get(), videoFrame))
            {
                auto packetEncode = encode(encodeContext, videoFrame);
                if(packetEncode)
                {
                    if (count >= 20000) {
                        ret = writePacket(packetEncode);
                        if (ret >= 0)
                        {
                            break;
                        }
                    }
                    cout<< count << endl;
                    count++;

                }

            }
        }
    }
    cout<<"got picture"<<endl;
    av_frame_free(&videoFrame);
    avcodec_close(encodeContext);
    Error:
    closeInput();
    closeOutput();
    return 0;

}
