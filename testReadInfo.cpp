//
// Created by 程方明 on 2019-04-29.
//

#include <iostream>

// 1.引用头文件
extern "C" {
#include<libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <sys/time.h>

#include <SDL.h>
#include <SDL_thread.h>
}
using namespace std;

static double r2d(AVRational r){
    return r.num == 0 || r.den == 0 ? 0.:(double)r.num/r.den;
}

long long GetNowMs(){
    struct  timeval tv;
    gettimeofday(&tv, NULL);
    int sec =tv.tv_sec%360000;
    long long t = sec*1000+tv.tv_sec /1000;
    return t;
}

int main(){
    char *url ="/Users/chengfangming/Downloads/311830.mp4";
//    av_log_set_level(AV_LOG_DEBUG);
    struct SwsContext *sws_ctx = NULL;
    av_register_all();
    avformat_network_init();
    avcodec_register_all();

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Event event;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;


    AVFormatContext *ic = avformat_alloc_context();
    int ret = avformat_open_input(&ic, url, NULL, NULL);
    if (ret < 0){
        return -1;
    }

    avformat_find_stream_info(ic,NULL);
    //av_dump_format(ic,0,url, false);
    int steamsLenght = ic->nb_streams;

    int videoIndex = -1;
    int audioIndex = -1;
    /**
     * 第一种方式
     */
     /**
    for (int i = 0; i < steamsLenght; ++i) {
        AVStream *avStream = ic->streams[i];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoIndex = i;
            cout<<"video info "<< avStream->codecpar->codec_id<<endl;
            cout<<"video fps "<< r2d(avStream->avg_frame_rate)<<endl;
            cout<<"video width "<< avStream->codecpar->width<<endl;
        }

        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            cout<<"audio codec_id "<< avStream->codecpar->codec_id<<endl;
            cout<<"audio sample_rate "<< avStream->codecpar->sample_rate<<endl;
            cout<<"audio format "<< avStream->codecpar->format<<endl;
        }
    }*/

    audioIndex = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    videoIndex = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    cout<<"audio av_find_best_stream "<< audioIndex <<endl;

    // 软解码
    AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoIndex]->codecpar->codec_id);
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioIndex]->codecpar->codec_id);
    // 硬解码
//    vcodec = avcodec_find_encoder_by_name("h264_mediacodec");
    if (!vcodec) {
        printf("vcodec find null\n");
        return -1;
    }

    if (!acodec) {
        printf("avcode find null\n");
        return -1;
    }


    // 解码器初始化
    AVCodecContext *vc = avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(vc, ic->streams[videoIndex]->codecpar);
    vc->thread_count = 8;

    // 打开解码器
    ret = avcodec_open2(vc, NULL, NULL);
    if (ret != 0) {
        printf("avcode open failed!\n");
    }

    // 解码器初始化
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(ac, ic->streams[audioIndex]->codecpar);
    ac->thread_count = 1;

    // 打开解码器
    ret = avcodec_open2(ac, NULL, NULL);
    if (ret != 0) {
        printf("audio open failed!\n");
    }
    screen = SDL_CreateWindow("My Game Window",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             vc->width, vc->height,
                              0);

    if(!screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }
    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }

    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            vc->width,
            vc->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }


    sws_ctx = sws_getContext(vc->width, vc->height,
                             vc->pix_fmt, vc->width, vc->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

    // set up YV12 pixel array (12 bits per pixel)
    yPlaneSz = vc->width * vc->height;
    uvPlaneSz = vc->width * vc->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = vc->width / 2;
    // 不能传空，
    // 内存分两部分，1部分是内存本身的空间，2，另一部分是对象内部数据的空间
    // 对象对象，并对象并减引用计数
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    long long start = GetNowMs();
    int frameCount = -1;
    for (;;) {
        int ret = av_read_frame(ic, pkt);

        if (ret != 0){
            cout<<"读取到结尾处 " <<endl;
//            int pos = 20 * r2d(ic->streams[videoIndex]->time_base);
//            av_seek_frame(ic, videoIndex, pos, AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
            break;
        }
        /**
        if (pkt->stream_index != videoIndex){
            continue;
        }*/
        AVCodecContext *cc = vc;
        if (pkt->stream_index != videoIndex){
            cc = ac;
        }
//        printf("stream = %d size = %d pts = %lld flag=%d \n",
//               pkt->stream_index, pkt->size, pkt->pts, pkt->flags);

        // 发送到线程中解码
        ret = avcodec_send_packet(cc, pkt);
        av_packet_unref(pkt);
        if (ret !=0 ) {
//            printf("send failed \n");
            continue;
        }
        for(;;){
            // 超过三秒
            if (GetNowMs() - start >= 3000){
                printf("now decodec fps is %d\n", frameCount / 3);
                start = GetNowMs();
                frameCount = 0;
            }
            ret = avcodec_receive_frame(cc, frame);
            if (ret != 0) {
//                printf("receive failed \n");
                break;
            }
//            printf("avcodec_receive_frame %lld\n", frame->pts);
            // 如果是视频帧
            if (cc == vc){
                frameCount++;

                AVPicture pict;
                pict.data[0] = yPlane;
                pict.data[1] = uPlane;
                pict.data[2] = vPlane;
                pict.linesize[0] = vc->width;
                pict.linesize[1] = uvPitch;
                pict.linesize[2] = uvPitch;

                // Convert the image into YUV format that SDL uses
                sws_scale(sws_ctx, (uint8_t const * const *) frame->data,
                          frame->linesize, 0, vc->height, pict.data,
                          pict.linesize);

                SDL_UpdateYUVTexture(
                        texture,
                        NULL,
                        yPlane,
                        vc->width,
                        uPlane,
                        uvPitch,
                        vPlane,
                        uvPitch
                );
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);

                SDL_PollEvent(&event);
                switch (event.type) {
                    case SDL_QUIT:
                        SDL_DestroyTexture(texture);
                        SDL_DestroyRenderer(renderer);
                        SDL_DestroyWindow(screen);
                        SDL_Quit();
                        exit(0);
                        break;
                    default:
                        break;
                }

            }

        }





        av_packet_unref(pkt);
    }
    av_packet_unref(pkt);
    avformat_close_input(&ic);

    return 0;
}