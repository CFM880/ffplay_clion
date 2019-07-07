//
// Created by 程方明 on 2019-06-17.
//

/**
 *                |---> 音频
 *                |
 * 网络包--解码-->|
 *                |
 *                |
 *                |--->视频
 *
 *
 */
#include <pch.h>

#include <string>
#include <thread>
#include <memory>
#include <iostream>
#include <stdio.h>

using namespace std;

AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext = nullptr;
int64_t lastReadPacketTime;


AVFilterContext *bufferSinkCtx;
AVFilterContext *bufferSrcCtx;
AVFilterGraph *filterGraph;

AVCodecContext *outPutAudioEncContext;

int64_t audioCount = 0;

/**
 * 多实例的时候可以传递当前实例
 * @param ctx
 * @return
 */
static int interrupt_cb(void *ctx) {
    int timeout = 10;
    if (av_gettime() - lastReadPacketTime > timeout * 1000 * 1000) {
        exit(-1);
    }
    return 0;
}



int OpenInput(string inputUrl) {
    inputContext = avformat_alloc_context();
    /**
     * 一个周期回调一次
     */
    lastReadPacketTime = av_gettime();
    inputContext->interrupt_callback.callback = interrupt_cb;


    AVInputFormat *ifmt = av_find_input_format("avfoundation");
    AVDictionary *formt_opts = nullptr;
//    av_dict_set_int(&formt_opts, "framerate", 30, 0);
//    av_dict_set(&formt_opts, "video_size", "640x480", 0);
//    av_dict_set(&formt_opts, "pixel_format", "uyvy422", 0);

//    av_log(NULL, AV_LOG_ERROR, avdevice_list_devices());
    int ret  = avformat_open_input(&inputContext, inputUrl.c_str(), ifmt, &formt_opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
        return ret;
    }
    ret = avformat_find_stream_info(inputContext, nullptr);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Find input file Stream info failed\n");
    } else {
        ret  = avcodec_open2(inputContext->streams[0]->codec,avcodec_find_decoder(inputContext->streams[0]->codec->codec_id),nullptr);
        av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n",inputUrl.c_str());
    }
    return ret;
}

int initAudioFilters() {
    char args[512];
    int ret;
    const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();


    auto audioDecoderContext = inputContext->streams[0]->codec;
    if (!audioDecoderContext->channel_layout) {
        audioDecoderContext->channel_layout = av_get_default_channel_layout(audioDecoderContext->channels);
    }

    static const enum AVSampleFormat out_sample_fmts[] = {AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE};
    static const int64_t out_channel_layouts[] = {static_cast<int64_t>(audioDecoderContext->channel_layout), -1};
    static const int out_sample_rates[] = {audioDecoderContext->sample_rate, -1};

    AVRational time_base = inputContext->streams[0]->time_base;
    filterGraph = avfilter_graph_alloc();
    filterGraph->nb_threads = 1;

    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            time_base.num, time_base.den, audioDecoderContext->sample_rate,
            av_get_sample_fmt_name(audioDecoderContext->sample_fmt), audioDecoderContext->channel_layout);

    ret = avfilter_graph_create_filter(&bufferSrcCtx, abuffersrc, "in", args, NULL, filterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sources\n");
        return ret;
    }

    ret = avfilter_graph_create_filter(&bufferSinkCtx, abuffersink, "out", NULL, NULL, filterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        return ret;
    }

    ret = av_opt_set_int_list(bufferSinkCtx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample_fmts\n");
        return ret;
    }

    ret = av_opt_set_int_list(bufferSinkCtx, "channel_layouts", out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel_layouts\n");
        return ret;
    }

    ret = av_opt_set_int_list(bufferSinkCtx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample_rates\n");
        return ret;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = bufferSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = bufferSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filterGraph, "anull", &inputs, &outputs, nullptr)) < 0) {
        return ret;
    }

    if ((ret = avfilter_graph_config(filterGraph, NULL)) < 0) {
        return ret;
    }

    av_buffersink_set_frame_size(bufferSinkCtx, 1024);
    return 0;
}


int InitAudioEncoderCodec(AVCodecContext *inputAudioContext) {
    int ret = 0;
    AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    outPutAudioEncContext = avcodec_alloc_context3(audioCodec);
    outPutAudioEncContext->codec = audioCodec;
    outPutAudioEncContext->sample_rate = inputAudioContext->sample_rate;
    outPutAudioEncContext->channel_layout = inputAudioContext->channel_layout;
    outPutAudioEncContext->channels = av_get_channel_layout_nb_channels(inputAudioContext->channel_layout);
    if (outPutAudioEncContext->channel_layout == 0) {
        outPutAudioEncContext->channel_layout = AV_CH_LAYOUT_STEREO;
        outPutAudioEncContext->channels = av_get_channel_layout_nb_channels(outPutAudioEncContext->channel_layout);
    }

    outPutAudioEncContext->sample_fmt = audioCodec->sample_fmts[0];
    outPutAudioEncContext->codec_tag = 0;
    outPutAudioEncContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ret = avcodec_open2(outPutAudioEncContext, audioCodec, 0);
    return ret;
}


AVFrame *DecodeAudio(AVPacket *packet, AVFrame *pSrcAudioFrame)
{
    AVStream *stream = inputContext->streams[0];
    AVCodecContext *codecContext = stream->codec;
    int gotFrame;
    AVFrame *filterFrame = nullptr;
    auto length = avcodec_decode_audio4(codecContext, pSrcAudioFrame, &gotFrame, packet);
    if (length >= 0 && gotFrame != 0) {
        if (av_buffersrc_add_frame_flags(bufferSrcCtx, pSrcAudioFrame, AV_BUFFERSRC_FLAG_PUSH < 0)){
            av_log(NULL, AV_LOG_ERROR, "buffer src add frame error!\n");
            return nullptr;
        }

        filterFrame = av_frame_alloc();
        int ret = av_buffersink_get_frame_flags(bufferSinkCtx, filterFrame, AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT);
        if (ret < 0) {
            av_frame_free(&filterFrame);
            goto error;
        }
        return filterFrame;
    }
    error:
    return nullptr;
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

int WritePacket(shared_ptr<AVPacket> packet){
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    av_interleaved_write_frame(outputContext, packet.get());
}

int OpenOut(string outUrl){
    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", outUrl.c_str());
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
        stream->codec = outPutAudioEncContext;
        AVCodecContext *codecContext = inputContext->streams[0]->codec;
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


void Init(){
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    avdevice_register_all();
    av_log_set_level(AV_LOG_ERROR);
}

int main(){
    Init();
    AVFrame *pSrcAudioFrame = av_frame_alloc();
    int ret = OpenInput(":0");
    int got_frame = 0;
    if (ret < 0 ) goto Error;

    ret = initAudioFilters();
    if (ret < 0 ) goto Error;
    ret = InitAudioEncoderCodec(inputContext->streams[0]->codec);
    if (ret < 0 ) goto Error;
    if (ret >= 0) {
        ret = OpenOut("/Users/chengfangming/Downloads/aac.aac");
    }

    if(ret <0) goto Error;

    while (true) {
        auto packet = ReadPacketFromSource();
        auto filterFrame = DecodeAudio(packet.get(), pSrcAudioFrame);
        if (filterFrame) {
            shared_ptr<AVPacket> pkt(static_cast<AVPacket *> (av_malloc(sizeof(AVPacket))),[&](AVPacket *p){av_packet_free(&p);});
            av_init_packet(pkt.get());
            pkt->data = nullptr;
            pkt->size = 0;

            ret = avcodec_encode_audio2(outPutAudioEncContext, pkt.get(), filterFrame, &got_frame);
            if (ret  >= 0 && got_frame > 0) {
                auto streamTimeBase = outputContext->streams[pkt->stream_index]->time_base.den;
                auto codecTimeBase = outputContext->streams[pkt->stream_index]->codec->time_base.den;
                pkt->pts = pkt->dts = (1024 * streamTimeBase * audioCount) / codecTimeBase;
                audioCount++;
            }

        }
        if (packet) {
            ret = WritePacket(packet);
            if (ret >= 0) {
                cout << "WritePacket Success" << packet->pos << endl;
            } else {
                cout << "WritePacket failed"<< endl;
            }
        }
    }

    Error:
    while (true) {
        return  -1;
    }

}
