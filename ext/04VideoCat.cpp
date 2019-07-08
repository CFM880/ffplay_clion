//
// Created by 程方明 on 2019-07-07.
//

#include <pch.h>

#include <string>
#include <iostream>
#include <thread>
#include <memory>


AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext = nullptr;
int64_t lastReadPacketTime;

void CloseInput();

void CloseOutput();

using namespace std;

static int interrupt_cb(void *ctx) {
    int timeout = 3;
    if (av_gettime() - lastReadPacketTime > timeout * 1000 * 1000) {
        exit(-1);
    }
    return 0;
}


int OpenInput(string inputUrl) {
    inputContext = avformat_alloc_context();
    lastReadPacketTime = av_gettime();
    /**
     * 一个周期回调一次
     */
    inputContext->interrupt_callback.callback = interrupt_cb;
    int ret  = avformat_open_input(&inputContext, inputUrl.c_str(), nullptr, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
        return ret;
    }
    ret = avformat_find_stream_info(inputContext, nullptr);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Find input file Stream info failed\n");
    } else {
        av_log(NULL, AV_LOG_FATAL, "Open input file %s success\n", inputUrl.c_str());
    }
    return ret;
}

int OpenOut(string outUrl){
    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "flv", outUrl.c_str());
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }
    ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }

    for (int i = 0; i < inputContext->nb_streams; ++i) {
        AVStream *stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
        ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "copy codec context failed");
            goto Error;
        }
    }
    ret = avformat_write_header(outputContext, nullptr);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "write header failed");
        goto Error;
    }

    av_log(NULL, AV_LOG_FATAL, "open output success %s\n", outUrl.c_str());
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


shared_ptr<AVPacket> ReadPacketFromSource(){
    lastReadPacketTime = av_gettime();
    shared_ptr<AVPacket> packet(static_cast<AVPacket *> (av_malloc(sizeof(AVPacket))),[&](AVPacket *p){av_packet_free(&p);});
    av_init_packet(packet.get());
    int ret = av_read_frame(inputContext, packet.get());
    if (ret >=0 ){
        return packet;
    } else {
        return nullptr;
    }
}

void Init(){
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);
}
int WritePacket(shared_ptr<AVPacket> packet){
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
//    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return av_interleaved_write_frame(outputContext, packet.get());
}

int main(){
    // 第20s开始，去掉8s
    int startPacketNum = 500;
    int discardPacketNum = 200;
    int packetCount = 0;
    int64_t lastPacketPts = AV_NOPTS_VALUE;
    int64_t lastPts = AV_NOPTS_VALUE;
    Init();
    int ret = OpenInput("/Users/chengfangming/Downloads/ttt1.flv");
    if (ret >= 0) {
        ret = OpenOut("/Users/chengfangming/Downloads/out.flv");
    }
    if (ret < 0 ) goto Error;
    while (true) {
        auto packet = ReadPacketFromSource();

        if (packet) {
            packetCount ++;
            cout << "WritePacket Success" << packetCount<< endl;
            if (packetCount <= 500 || packetCount >=700) {
                if (packetCount >=700) {
                    if (packet->pts - lastPacketPts >  120){
                        lastPts = lastPacketPts;
                    } else {
                        auto diff = packet->pts - lastPacketPts;
                        lastPts += diff;
                    }
                }
                lastPacketPts = packet->pts;
                if (lastPts != AV_NOPTS_VALUE) {
                    packet->pts = packet->pts = lastPts;
                }
                ret = WritePacket(packet);

            }
        }
    }

    Error:
    CloseInput();
    CloseOutput();
    while (true) {
        return  -1;
    }

}

void CloseOutput() {
    if (outputContext != nullptr) {
        for (int i = 0; i < outputContext->nb_streams; ++i) {
            AVCodecContext *codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&outputContext);
    }
}

void CloseInput() {
    if (inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}