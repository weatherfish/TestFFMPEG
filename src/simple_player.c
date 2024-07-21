#include <libavutil/log.h>
#include <SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct _VideoState {
    AVCodecContext *avCtx;
    AVPacket *pkt;
    AVFrame *frame;
    SDL_Texture *texture;
} VideoState;

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;
static int width = 640, height = 480;

static void render(VideoState *vs) {
    SDL_UpdateYUVTexture(vs->texture, NULL, vs->frame->data[0], vs->frame->linesize[0], 
                         vs->frame->data[1], vs->frame->linesize[1], 
                         vs->frame->data[2], vs->frame->linesize[2]);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, vs->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static int decode(VideoState *vs) {
    int ret = avcodec_send_packet(vs->avCtx, vs->pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(vs->avCtx, vs->frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during decoding!\n");
            return ret;
        }
        render(vs);
    }
    return 0;
}

int simple_player(int argc, char* argv[]) {
    if (argc < 2) {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 2!\n");
        return -1;
    }
    char* src = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    AVFormatContext *fmtCtx = NULL;
    if (avformat_open_input(&fmtCtx, src, NULL, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open source file %s\n", src);
        goto cleanup;
    }

    if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
        goto cleanup;
    }

    int video_stream_index = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find video stream in the input, aborting\n");
        goto cleanup;
    }

    AVStream *video_stream = fmtCtx->streams[video_stream_index];
    const AVCodec *codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find codec\n");
        goto cleanup;
    }

    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate the codec context\n");
        goto cleanup;
    }

    if (avcodec_parameters_to_context(codecCtx, video_stream->codecpar) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to decoder context\n");
        goto cleanup;
    }

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open codec\n");
        goto cleanup;
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    if (!pkt || !frame) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate frame or packet\n");
        goto cleanup;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codecCtx->width, codecCtx->height);
    if (!texture) {
        av_log(NULL, AV_LOG_ERROR, "Could not create texture: %s\n", SDL_GetError());
        goto cleanup;
    }

    VideoState vs = { codecCtx, pkt, frame, texture };

    SDL_Event event;
    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            decode(&vs);
        }
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        av_packet_unref(pkt);
    }

    vs.pkt = NULL;
    decode(&vs);

cleanup:
    if (frame) av_frame_free(&frame);
    if (pkt) av_packet_free(&pkt);
    if (codecCtx) avcodec_free_context(&codecCtx);
    if (fmtCtx) avformat_close_input(&fmtCtx);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
