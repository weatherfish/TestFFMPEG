#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/fifo.h>
#include <libswresample/swresample.h>
#include <SDL.h>

typedef struct _PacketQueue {
    AVFifo *pkts;
    int nb_packets;
    int size;
    int64_t duration;

    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

typedef struct _MyPacketElement {
    AVPacket *pkt;
} MyPacketElement;

typedef struct _MediaState {
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
} MediaState;

#define AUDIO_BUFFER_SIZE 1024
static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;
static int width = 640, height = 480;

static int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->pkts = av_fifo_alloc2(1, sizeof(MyPacketElement), AV_FIFO_FLAG_AUTO_GROW);
    if (!q->pkts) {
        return AVERROR(ENOMEM);
    }
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int packet_queue_put_priv(PacketQueue *q, AVPacket *pkt) {
    MyPacketElement elem;
    int ret = -1;
    elem.pkt = pkt;
    ret = av_fifo_write(q->pkts, &elem, 1);
    if (ret < 0) {
        return ret;
    }
    q->nb_packets++;
    q->size += elem.pkt->size + sizeof(elem);
    q->duration += elem.pkt->duration;

    SDL_CondSignal(q->cond);

    return ret;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacket *pkt1;
    int ret;
    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_priv(q, pkt1);
    SDL_UnlockMutex(q->mutex);

    if (ret < 0) {
        av_packet_unref(pkt);
        return -1;
    }
    return ret;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    MyPacketElement *element;
    int ret = -1;

    SDL_LockMutex(q->mutex);
    for (;;) {
        if (av_fifo_read(q->pkts, &element, 1) >= 0) {
            q->nb_packets--;
            q->size -= element->pkt->size + sizeof(element);
            q->duration -= element->pkt->duration;
            av_packet_move_ref(pkt, element->pkt);
            av_packet_free(&element->pkt);
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

static void packet_queue_flush(PacketQueue *q) {
    MyPacketElement elem;
    SDL_LockMutex(q->mutex);
    while (av_fifo_read(q->pkts, &elem, 1) > 0) {
        av_packet_free(&elem.pkt);
    }
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;

    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q) {
    packet_queue_flush(q);
    av_fifo_freep2(&q->pkts);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

static void render(MediaState *ms) {
    SDL_UpdateYUVTexture(ms->texture, NULL, ms->vframe->data[0], ms->vframe->linesize[0], ms->vframe->data[1],
                         ms->vframe->linesize[1], ms->vframe->data[2], ms->vframe->linesize[2]);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, ms->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static int decode(MediaState *ms) {
    int ret = -1;

    ret = avcodec_send_packet(ms->vCtx, ms->vpkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto __OUT;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(ms->vCtx, ms->vframe);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
            goto __OUT;
        } else if (ret < 0) {
            ret = -1;
            goto __OUT;
        }
        render(ms);
    }

__OUT:
    return ret;
}

static int audio_decode_frame(MediaState *ms) {
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
    MediaState *ms = (MediaState *)userdata;
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

int simple_player2(int argc, char *argv[]) {
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

    av_log_set_level(AV_LOG_DEBUG);

    // 1. 判断输入参数
    if (argc < 2) {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 2!\n");
        exit(-1);
    }
    src = argv[1];

    // 2. 初始化SDL，并创建窗口和Render
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        av_log(NULL, AV_LOG_ERROR, "无法初始化 SDL: %s\n", SDL_GetError());
        goto __END;
    }
    win = SDL_CreateWindow("SimplePlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win) {
        av_log(NULL, AV_LOG_ERROR, "无法创建窗口: %s\n", SDL_GetError());
        SDL_Quit();
        goto __END;
    }

    renderer = SDL_CreateRenderer(win, -1, 0);
    if (!renderer) {
        av_log(NULL, AV_LOG_ERROR, "无法创建渲染器: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        goto __END;
    }

    // 3. 打开多媒体，获取流信息
    if ((ret = avformat_open_input(&fmtCtx, src, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, " %s\n", av_err2str(ret));
        goto __END;
    }

    if ((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, " %s\n", av_err2str(ret));
        goto __END;
    }

    // 4. 查找最好的视频流
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
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find video stream \n");
        goto __END;
    }
    if (aidx == -1) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find audio stream \n");
        goto __END;
    }

    // 5. 根据流中的codec_id获取编解码器
    ainStream = fmtCtx->streams[aidx];
    vinStream = fmtCtx->streams[vidx];

    vcodec = avcodec_find_decoder(vinStream->codecpar->codec_id);
    if (!vcodec) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find video codec: %u\n", vinStream->codecpar->codec_id);
        goto __END;
    }
    acodec = avcodec_find_decoder(ainStream->codecpar->codec_id);
    if (!acodec) {
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find audio codec: %u\n", ainStream->codecpar->codec_id);
        goto __END;
    }

    // 6. 创建解码器上下文
    vctx = avcodec_alloc_context3(vcodec);
    if (!vctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc video context for: %s\n", vcodec->long_name);
        goto __END;
    }
    actx = avcodec_alloc_context3(acodec);
    if (!actx) {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc audio context for: %s\n", acodec->long_name);
        goto __END;
    }

    // 7. 从视频流拷贝编码器参数到解码器
    ret = avcodec_parameters_to_context(vctx, vinStream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not copy video codecpar to ctx context\n");
        goto __END;
    }
    ret = avcodec_parameters_to_context(actx, ainStream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not copy audio codecpar to ctx context\n");
        goto __END;
    }

    // 8. 绑定解码器上下文
    ret = avcodec_open2(vctx, vcodec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open video codecpar: %s\n", av_err2str(ret));
        goto __END;
    }
    ret = avcodec_open2(actx, acodec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open audio codecpar: %s\n", av_err2str(ret));
        goto __END;
    }

    // 9. 根据视频宽高创建纹理
    apkt = av_packet_alloc();
    aframe = av_frame_alloc();

    vpkt = av_packet_alloc();
    vframe = av_frame_alloc();

    packet = av_packet_alloc();

    int video_width = vctx->width;
    int video_height = vctx->height;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);

    MediaState *ms = NULL;
    ms = malloc(sizeof(MediaState));
    if (!ms) {
        av_log(NULL, AV_LOG_ERROR, "Could not malloc VideoState\n");
        goto __END;
    }
    memset(ms, 0, sizeof(MediaState));

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
        goto __END;
    }

    SDL_PauseAudio(0);

    // 10. 从多媒体文件读取数据，进行解码
    while (av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index == vidx) {
            av_packet_move_ref(ms->vpkt, packet);
            decode(ms); // 11. 解码和渲染视频流
        } else if (packet->stream_index == aidx) {
            packet_queue_put(&ms->audioQueue, packet); // 音频流
        } else {
            av_packet_unref(packet);
        }
         // 12. SDL事件处理
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                goto __QUIT;
                break;

            default:
                break;
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

__QUIT:
    ret = 0;
    // 13. 收尾
__END:
    if (aframe) {
        av_frame_free(&aframe);
    }
    if (vframe) {
        av_frame_free(&vframe);
    }
    if (apkt) {
        av_packet_free(&apkt);
    }
    if (vpkt) {
        av_packet_free(&vpkt);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (actx) {
        avcodec_free_context(&actx);
    }
    if (vctx) {
        avcodec_free_context(&vctx);
    }
    if (fmtCtx) {
        avformat_close_input(&fmtCtx);
    }
    if (win) {
        SDL_DestroyWindow(win);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (ms) {
        free(ms);
    }
    SDL_Quit();

    return ret;
}
