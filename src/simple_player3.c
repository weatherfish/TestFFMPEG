#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
#include "queue.h"
#include <string.h>

typedef struct _VideoState {
    AVCodecContext *aCtx;
    AVCodecContext *vCtx;
    SwrContext *swr_ctx;

    uint8_t *audio_buf;
    uint audio_buf_size;
    int audio_buf_index;

    AVPacket *apkt;
    AVPacket *vpkt;
    AVFrame *aframe;
    AVFrame *vframe;
    SDL_Texture *texture;

    PacketQueue audioQueue;
} VideoState;

#define AUDIO_BUFFER_SIZE 1024
static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;
static int width = 640, height = 480;

static int audio_decode_frame(VideoState *ms) {
    AVPacket pkt;
    int ret = -1;
    int len2 = -1;
    int data_size = 0;
    for (;;) {
        if (packet_queue_get(&ms->audioQueue, &pkt, 1) < 0) {
            return -1;
        }

        ret = avcodec_send_packet(ms->aCtx, &pkt);
        if (ret < 0) {
            av_log(ms->aCtx, AV_LOG_ERROR, "Failed to send packet to audio codec %s\n", av_err2str(ret));
            goto __OUT;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(ms->aCtx, ms->aframe);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                av_log(ms->aCtx, AV_LOG_ERROR, "Failed to receive frame from audio decoder %s\n", av_err2str(ret));
                goto __OUT;
            }
            if (!ms->swr_ctx) {
                AVChannelLayout in_ch_layout;
                AVChannelLayout out_ch_layout;
                av_channel_layout_copy(&in_ch_layout, &ms->aCtx->ch_layout);
                av_channel_layout_copy(&out_ch_layout, &in_ch_layout);

                if (ms->aCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
                    swr_alloc_set_opts2(&ms->swr_ctx, &out_ch_layout, AV_SAMPLE_FMT_S16, ms->aCtx->sample_rate,
                                        &in_ch_layout, ms->aCtx->sample_fmt, ms->aCtx->sample_rate, 0, NULL);
                    swr_init(ms->swr_ctx);
                }
            }

            if (ms->swr_ctx) {
                const uint8_t **in = (const uint8_t **)ms->aframe->extended_data;
                unsigned char *const *out = (unsigned char *const *)ms->audio_buf;
                int out_count = ms->aframe->nb_samples + 512;

                int out_size = av_samples_get_buffer_size(NULL, ms->aframe->ch_layout.nb_channels, out_count, AV_SAMPLE_FMT_S16, 0);
                av_fast_malloc(&ms->audio_buf, &ms->audio_buf_size, out_size);

                len2 = swr_convert(ms->swr_ctx, out, out_size, in, ms->aframe->nb_samples);

                data_size = len2 * ms->aframe->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            } else {
                ms->audio_buf = ms->aframe->data[0];
                data_size = av_samples_get_buffer_size(NULL, ms->aframe->ch_layout.nb_channels, ms->aframe->nb_samples, AV_SAMPLE_FMT_S16, 1);
            }
            av_packet_unref(&pkt);
            av_frame_unref(ms->aframe);

            return data_size;
        }
    }

__OUT:
    return ret;
}

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    int audio_size = 0;
    VideoState *ms = (VideoState *)userdata;
    while (len > 0) {
        if (ms->audio_buf_index >= ms->audio_buf_size) {
            audio_size = audio_decode_frame(ms);
            if (audio_size < 0) {
                ms->audio_buf_size = AUDIO_BUFFER_SIZE;
                ms->audio_buf = NULL;
            } else {
                ms->audio_buf_size = audio_size;
            }
            ms->audio_buf_index = 0;
        }
        int current_len = ms->audio_buf_size - ms->audio_buf_index;
        if (current_len > len) {
            current_len = len;
        }
        if (ms->audio_buf) {
            memcpy(stream, (uint8_t *)ms->audio_buf + ms->audio_buf_index, current_len);
        } else {
            memset(stream, 0, current_len);
        }
        len -= current_len;
        stream += current_len;
        ms->audio_buf_index += current_len;
    }
}

static void render(VideoState *vs) {
    SDL_UpdateYUVTexture(vs->texture, NULL, vs->vframe->data[0], vs->vframe->linesize[0], 
                         vs->vframe->data[1], vs->vframe->linesize[1], 
                         vs->vframe->data[2], vs->vframe->linesize[2]);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, vs->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static int decode(VideoState *vs) {
    int ret = avcodec_send_packet(vs->vCtx, vs->vpkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send packet to decoder: %s\n", av_err2str(ret));
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(vs->vCtx, vs->vframe);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during decoding: %s\n", av_err2str(ret));
            return ret;
        }
        render(vs);
    }
    return 0;
}

int simple_player3(int argc, char* argv[]) {

    char *src;
    int ret = -1;
    int vidx = -1;
    int aidx = -1;

    AVFormatContext *fmtCtx = NULL;
    AVCodecContext *vctx = NULL;
    AVCodecContext *actx = NULL;
    AVStream *ainStream = NULL;
    AVStream *vinStream = NULL;
    const AVCodec *vcodec;
    const AVCodec *acodec;

    AVPacket *packet = NULL;
    AVPacket *apkt = NULL;
    AVPacket *vpkt = NULL;
    AVFrame *aframe = NULL;
    AVFrame *vframe = NULL;

    SDL_Texture *texture = NULL;
    SDL_Event event;

    if (argc < 2) {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 2!\n");
        return -1;
    }
    src = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    win = SDL_CreateWindow("SimplePlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                           width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win) {
        av_log(NULL, AV_LOG_ERROR, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(win, -1, 0);
    if (!renderer) {
        av_log(NULL, AV_LOG_ERROR, "Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return -1;
    }

    if (avformat_open_input(&fmtCtx, src, NULL, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open source file %s\n", src);
        goto cleanup;
    }

        if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
        goto cleanup;
    }

    for (size_t i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vidx < 0) {
            vidx = i;
        }
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && aidx < 0) {
            aidx = i;
        }
        if (vidx > -1 && aidx > -1) {
            break;
        }
    }

    if (vidx == -1) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find video stream\n");
        goto cleanup;
    }
    if (aidx == -1) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find audio stream\n");
        goto cleanup;
    }

    ainStream = fmtCtx->streams[aidx];
    vinStream = fmtCtx->streams[vidx];

    vcodec = avcodec_find_decoder(vinStream->codecpar->codec_id);
    if (!vcodec) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find video codec: %u\n", vinStream->codecpar->codec_id);
        goto cleanup;
    }
    acodec = avcodec_find_decoder(ainStream->codecpar->codec_id);
    if (!acodec) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find audio codec: %u\n", ainStream->codecpar->codec_id);
        goto cleanup;
    }

    vctx = avcodec_alloc_context3(vcodec);
    if (!vctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc video context for: %s\n", vcodec->long_name);
        goto cleanup;
    }
    actx = avcodec_alloc_context3(acodec);
    if (!actx) {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc audio context for: %s\n", acodec->long_name);
        goto cleanup;
    }

    // 从视频流拷贝编码器参数到解码器
    ret = avcodec_parameters_to_context(vctx, vinStream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not copy video codecpar to ctx context\n");
        goto cleanup;
    }
    ret = avcodec_parameters_to_context(actx, ainStream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not copy audio codecpar to ctx context\n");
        goto cleanup;
    }

    // 绑定解码器上下文
    ret = avcodec_open2(vctx, vcodec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open video codecpar: %s\n", av_err2str(ret));
        goto cleanup;
    }
    ret = avcodec_open2(actx, acodec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open audio codecpar: %s\n", av_err2str(ret));
        goto cleanup;
    }

    // 根据视频宽高创建纹理
    apkt = av_packet_alloc();
    aframe = av_frame_alloc();
    vpkt = av_packet_alloc();
    vframe = av_frame_alloc();
    packet = av_packet_alloc();

    if (!vpkt || !vframe || !apkt || !aframe || !packet) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate frame or packet\n");
        goto cleanup;
    }

    int video_width = vctx->width;
    int video_height = vctx->height;

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);
    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "Could not create texture: %s\n", SDL_GetError());
        goto cleanup;
    }

    VideoState *ms = malloc(sizeof(VideoState));
    if (!ms) {
        av_log(NULL, AV_LOG_ERROR, "Could not malloc VideoState\n");
        goto cleanup;
    }
    memset(ms, 0, sizeof(VideoState));

    packet_queue_init(&ms->audioQueue);

    ms->vCtx = vctx;
    ms->aCtx = actx;
    ms->texture = texture;
    ms->apkt = apkt;
    ms->vpkt = vpkt;
    ms->aframe = aframe;
    ms->vframe = vframe;
    ms->audio_buf = NULL;
    ms->audio_buf_size = 0;
    ms->audio_buf_index = 0;
    ms->swr_ctx = NULL;

    // 设置音频设备参数
    SDL_AudioSpec wanted_spec, spec;
    wanted_spec.freq = actx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = actx->ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = AUDIO_BUFFER_SIZE;
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = (void *)ms;

    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open audio device\n");
        goto cleanup;
    }

    SDL_PauseAudio(0);

    // 从多媒体文件读取数据，进行解码
    while (av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index == vidx) {
            av_packet_move_ref(ms->vpkt, packet);
            decode(ms); // 解码和渲染视频流
        } else if (packet->stream_index == aidx) {
            packet_queue_put(&ms->audioQueue, packet); // 音频流
        } else {
            av_packet_unref(packet);
        }

        // SDL事件处理
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            goto cleanup;
        }
    }

    // 刷新音频解码器
    avcodec_send_packet(ms->aCtx, NULL);
    while (avcodec_receive_frame(ms->aCtx, ms->aframe) == 0) {
        // 处理剩余的音频帧
    }

    // 刷新视频解码器
    avcodec_send_packet(ms->vCtx, NULL);
    while (avcodec_receive_frame(ms->vCtx, ms->vframe) == 0) {
        render(ms); // 渲染剩余的视频帧
    }

    ms->vpkt = NULL;
    decode(ms);

cleanup:
    if (vframe) av_frame_free(&vframe);
    if (aframe) av_frame_free(&aframe);
    if (vpkt) av_packet_free(&vpkt);
    if (apkt) av_packet_free(&apkt);
    if (packet) av_packet_free(&packet);
    if (vctx) avcodec_free_context(&vctx);
    if (actx) avcodec_free_context(&actx);
    if (fmtCtx) avformat_close_input(&fmtCtx);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (win) SDL_DestroyWindow(win);
    if (ms) free(ms);
    SDL_Quit();
    return 0;
}
