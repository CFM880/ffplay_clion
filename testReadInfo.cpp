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
#include <libavutil/time.h>

#include <SDL.h>
#include <SDL_thread.h>
}
using namespace std;
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000
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
typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}
int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

    AVPacketList *pkt1;
    if(av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = static_cast<AVPacketList *>(av_malloc(sizeof(AVPacketList)));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;


    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for(;;) {

        if(quit) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;

    int len1, data_size = 0;

    for(;;) {
        while(audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
            if(len1 < 0) {
                /* if error, skip frame */
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            data_size = 0;
            if(got_frame) {
                data_size = av_samples_get_buffer_size(NULL,
                                                       aCodecCtx->channels,
                                                       frame.nb_samples,
                                                       aCodecCtx->sample_fmt,
                                                       1);
                assert(data_size <= buf_size);
                memcpy(audio_buf, frame.data[0], data_size);
            }
            if(data_size <= 0) {
                /* No data yet, get more frames */
                continue;
            }
            /* We have data, return it and come back for more later */
            return data_size;
        }
        if(pkt.data)
            av_free_packet(&pkt);

        if(quit) {
            return -1;
        }

        if(packet_queue_get(&audioq, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

    AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0) {
        if(audio_buf_index >= audio_buf_size) {
            /* We have already sent all our data; get more */
            audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
            if(audio_size < 0) {
                /* If error, output silence */
                audio_buf_size = 1024; // arbitrary?
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(){
    char *url ="/Users/chengfangming/Downloads/ffmpeg.flv";
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
    SDL_AudioSpec   wanted_spec, spec;

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
    AVCodecContext *aCodecCtx =avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(aCodecCtx, ic->streams[videoIndex]->codecpar);



    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = aCodecCtx;


    // 打开解码器
    ret = avcodec_open2(aCodecCtx, NULL, NULL);
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
    long long last_time = 0;
    int frameCount = -1;
    for (;;) {
        int ret = av_read_frame(ic, pkt);

        if (ret != 0) {
            cout << "读取到结尾处 " << endl;
//            int pos = 20 * r2d(ic->streams[videoIndex]->time_base);
//            av_seek_frame(ic, videoIndex, pos, AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
            break;
        }
        /**
        if (pkt->stream_index != videoIndex){
            continue;
        }*/
        AVCodecContext *cc = vc;
        if (pkt->stream_index != videoIndex) {
            cc = ac;
        }
//        printf("stream = %d size = %d pts = %lld flag=%d \n",
//               pkt->stream_index, pkt->size, pkt->pts, pkt->flags);

        // 发送到线程中解码
        ret = avcodec_send_packet(cc, pkt);
        av_packet_unref(pkt);
        if (ret != 0) {
//            printf("send failed \n");
            continue;
        }
        for (;;) {
            // 超过三秒
            if (GetNowMs() - start >= 3000) {
                printf("now decodec fps is %d\n", frameCount / 3);
                start = GetNowMs();
                frameCount = 0;
            }
            ret = avcodec_receive_frame(cc, frame);
            if (ret != 0) {
//                printf("receive failed \n");
                break;
            }

            // 如果是视频帧
            if (cc == vc) {

                printf("avcodec_receive_frame %lld\n", frame->pts);
                last_time = frame->pts;

                frameCount++;
                last_time = frame->pts;
                AVPicture pict;
                pict.data[0] = yPlane;
                pict.data[1] = uPlane;
                pict.data[2] = vPlane;
                pict.linesize[0] = vc->width;
                pict.linesize[1] = uvPitch;
                pict.linesize[2] = uvPitch;

                // Convert the image into YUV format that SDL uses
                sws_scale(sws_ctx, (uint8_t const *const *) frame->data,
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

            } else {
                packet_queue_put(&audioq, pkt);
            }

            SDL_PollEvent(&event);
            switch (event.type)
                case SDL_QUIT: {
                    SDL_DestroyTexture(texture);
                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(screen);
                    SDL_Quit();
                    exit(0);
                    break;
                }



            av_packet_unref(pkt);
        }
    }

    avformat_close_input(&ic);

    return 0;
}