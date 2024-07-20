#include <libavutil/log.h>
#include <SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct _VideoState{
    AVCodecContext * avCtx;
    AVPacket *pkt;
    AVFrame *frame;
    SDL_Texture * texture;
}VideoState;

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;
static int width =  640, height = 480;

static void render(VideoState *vs){

    SDL_UpdateYUVTexture(vs->texture, NULL, vs->frame->data[0], vs->frame->linesize[0], vs->frame->data[1],
         vs->frame->linesize[1],  vs->frame->data[2], vs->frame->linesize[2]);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, vs->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static int decode(VideoState *vs){
    int ret = -1;
    char bud[1024];

    ret = avcodec_send_packet(vs->avCtx, vs->pkt);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto __OUT;
    }
    while (ret > 0){
        ret = avcodec_receive_frame(vs->avCtx, vs->frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            ret = 0;
            goto __OUT;
        }else if( ret < 0){
            ret = -1;
            goto __OUT;
        }
        //播放
        render(vs);
    }
    

__OUT:
    return ret;

}

int simple_palyer(int argc, char* argv[]){
    char* src;
    int ret = -1;
    int idx = -1;
    AVFormatContext *fmtCtx = NULL;
    AVCodecContext *ctx = NULL;
    AVStream *inStream = NULL;
    const AVCodec *codec;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;

    SDL_Texture *texture = NULL;
    SDL_Event event;

    av_log_set_level(AV_LOG_DEBUG); 

    //1 判断输入参数
    if (argc < 2){
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 2!\n");
        exit(-1);
    }
    src = argv[1];


    //2 初始化SDL，并创建窗口和Render
    if (SDL_Init(SDL_INIT_VIDEO)){
        av_log(NULL, AV_LOG_ERROR, "Could not initialize SDL %s\n", SDL_GetError());
        return -1;
    }
    win = SDL_CreateWindow("SimplePlayer",SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        width, height, SDL_WINDOW_OPENGL| SDL_WINDOW_RESIZABLE);
    if(!win){
        av_log(NULL, AV_LOG_ERROR, "Could not initialize SDL %s\n", SDL_GetError());
        goto __END;
    }

    renderer = SDL_CreateRenderer(win, -1, 0);

    //3 打开多媒体，获取流信息
    if((ret = avformat_open_input(&fmtCtx, src, NULL, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, " %s\n", av_err2str(ret));
        goto __END;
    }
    
    if((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, " %s\n", av_err2str(ret));
        goto __END;
    }
    //4 查找最好的视频流
    idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(idx < 0){
        av_log(fmtCtx, AV_LOG_ERROR, "Does not find audio stream\n");
        goto __END;
    }

    //5 根据流中的codec_id获取编解码器
    inStream = fmtCtx->streams[idx];
    codec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if(!codec){
        av_log(fmtCtx, AV_LOG_ERROR, "Could not find codec: %s\n", inStream->codecpar->codec_id);
        goto __END;
    }

    //6 c创建解码器上下文
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
        av_log(NULL, AV_LOG_ERROR, "Could not alloc context for: %s\n", codec);
        goto __END;
    }

    //7 从视频流拷贝编码器参数到解码器
    ret = avcodec_parameters_to_context(ctx, inStream->codecpar);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Could not copy codecpar to ctx context\n");
        goto __END;
    }

    //8 绑定解码器上下文
    ret = avcodec_open2(ctx, codec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Could not open codecpar: %s\n", av_err2str(ret));
        goto __END;
    }

    //9 根据视频宽高创建纹理
    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    int video_width = ctx->width;
    int video_height = ctx->height;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);

    VideoState *vs = NULL;
    vs = malloc(sizeof(VideoState));
    if(!vs){
        av_log(NULL, AV_LOG_ERROR, "Could not malloc VideoState\n");
        goto __END;
    }
    vs->avCtx = ctx;
    vs->texture = texture;
    vs->pkt = pkt;
    vs->frame = frame;
    //10 从多媒体文件读取数据，进行解码
    while (av_read_frame(fmtCtx, pkt) >= 0){
        if(pkt->stream_index == idx){
            decode(vs); //11.解码和渲染
        }
        //12 SDL事件处理
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                goto __QUIT;
                break;
            
            default:
                break;
        }
        av_packet_unref(pkt);
    }
    vs->pkt = NULL;
    decode(vs);
    
__QUIT:
    ret = 0;
    //13 收尾
__END:
    if(frame){
        av_frame_free(&frame);
    }
    if(pkt){
        av_packet_free(&pkt);
    }
    if(ctx){
        avcodec_free_context(&ctx);
    }
    if(fmtCtx){
        avformat_close_input(&fmtCtx);
    }
    if(win){
        SDL_DestroyWindow(win);
    }
    if(renderer){
        SDL_DestroyRenderer(renderer);
    }
    if(texture){
        SDL_DestroyTexture(texture);
    }
    SDL_Quit();

    return ret;

}
