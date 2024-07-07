#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBIMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHADER;


static void save_pic(unsigned char* buf, int linesize, int width, int height, char* fileName){
    FILE *f = fopen(fileName, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);
    for (size_t i = 0; i < height; i++){       
        fwrite(buf + i * linesize, 1, width, f);

    }
    fclose(f);
}

static void save_bmp(struct SwsContext *swsContext, AVFrame* frame, int w, int h, char* name){
    
    int dataSize = w * h * 3;
    //1.将yuv转成bgr24
    AVFrame *frameBGR = av_frame_alloc();
    frameBGR->width =w;
    frameBGR->height = h;
    frameBGR->format = AV_PIX_FMT_BGR24;

    av_frame_get_buffer(frameBGR, 0);//分配空间
    
    sws_scale(swsContext, (const uint8_t* const*)frame->data,frame->linesize,
            0, frame->height, frameBGR->data, frameBGR->linesize);

    //2.构造 BITMAPINFOHEADER
    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = w;
    infoHeader.biHeight = h * (-1);
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = 0;
    infoHeader.biClrImportant = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biPlanes = 0;

    //3，构造 BITMAPFILEHEADER
    BITMAPFILEHEADER fileHader;
    fileHader.bfType = 0x4d42; //"BM"
    fileHader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataSize;
    fileHader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //4 数据写文件
    FILE *f = fopen(name, "wb");
    fwrite(&fileHader,  sizeof(BITMAPFILEHEADER), 1, f);
    fwrite(&infoHeader,  sizeof(BITMAPINFOHEADER), 1, f);
    fwrite(frameBGR->data[0], 1, dataSize, f);
    
    //5 释放资源
    fclose(f);
    av_freep(&frameBGR->data[0]);
    av_free(frameBGR);
}

static int decode(AVCodecContext *ctx, struct SwsContext *swsContext, AVFrame *frame, AVPacket *pkt, const char* fileName){
    int ret = -1;
    char buf[1024];
    ret = avcodec_send_packet(ctx, pkt);
    if(ret<0){
        av_log(NULL, AV_LOG_ERROR, "发送 packet 失败\n");
        goto _END;
    }

    while (ret >= 0){
        ret =avcodec_receive_frame(ctx, frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){
            return -1;//返回-1表示失败，需要退出程序
        }
        // snprintf(buf, sizeof(buf), "%s-%ld.jpg", fileName, ctx->frame_num);
        snprintf(buf, sizeof(buf), "%s-%ld.bmp", fileName, ctx->frame_num);
        save_bmp(swsContext, frame, frame->width, frame->width, buf);
        // save_pic(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
        if(pkt)av_packet_unref(pkt);
    }
_END:
    return 0;
}

int gen_pic2(int argc, char *argv[]) {
    // 1. 处理参数ls
    char *src;
    char *dst;

    int ret = -1;
    int videoIndex = -1;
    AVFormatContext *formatContext = NULL;
    AVStream *inStream = NULL;
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;

    const AVCodec *codec;
    AVCodecContext *codecContext = NULL;

    struct SwsContext * swsContext = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    if (argc < 3) {
        av_log(NULL, AV_LOG_INFO, "参数必须多于两个\n");
        exit(-1);
    }

    src = argv[1];
    dst = argv[2];

    // 2. 打开多媒体文件
    ret = avformat_open_input(&formatContext, src, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }

    // 3. 找到视频流
    videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoIndex < 0) {
        av_log(formatContext, AV_LOG_ERROR, "不包含视频流\n");
        goto _ERROR;
    }

    inStream = formatContext->streams[videoIndex];

    //4 查找解码器  通过id  通过名称 
    codec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if (!codec){
        av_log(NULL, AV_LOG_ERROR, "无法为 %d 找到Codec\n", inStream->codecpar->codec_tag);
        goto _ERROR;
    }

    //5 创建解码器上下文
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext){
        av_log(NULL, AV_LOG_ERROR, "内存不够, 调用avcodec_alloc_context3失败\n");
        goto _ERROR;
    }

    //5 绑定解码器和上下文
    ret = avcodec_open2(codecContext, codec, NULL);
    if ((ret < 0))
    {
        av_log(codecContext, AV_LOG_ERROR, "无法打开codec: %s\n", av_err2str(ret));
        goto _ERROR;
    }

    avcodec_parameters_to_context(codecContext, inStream->codecpar);

    //获取SWS上下文
    // swsContext = sws_getCachedContext(NULL, codecContext->width, codecContext->height, codecContext->pix_fmt,
    //            codecContext->width, codecContext->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
    swsContext = sws_getCachedContext(NULL, codecContext->width, codecContext->height, codecContext->pix_fmt,
               640, 320, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);


    //6 创建AVFrame  元数据
   frame = av_frame_alloc();
    if(!frame){
         av_log(NULL, AV_LOG_ERROR, "内存不足 av_frame_alloc\n");
        goto _ERROR;
    }

    //7 创建AVPacket  编码后数据
    packet = av_packet_alloc();
    if(!packet){
        av_log(NULL, AV_LOG_ERROR, "内存不足 av_packet_alloc\n");
        goto _ERROR;
    } 

    // 8. 从源文件读取视频数据到目标文件
    while (av_read_frame(formatContext, packet) >= 0) {
        if(packet->stream_index == videoIndex){
            decode(codecContext, swsContext, frame, packet, dst);
        }
    }
    //强制输出缓存中数据
    decode(codecContext, swsContext, frame, NULL, dst);


    // 9. 释放资源
_ERROR:
    if (formatContext) {
        avformat_free_context(formatContext);
        formatContext = NULL;
    }
    if(codecContext){
        avcodec_free_context(&codecContext);
        codecContext = NULL;
    }
    if(swsContext){
        
    }
    if(frame){
        av_frame_free(&frame);
        frame = NULL;
    }
    if(packet){
        av_packet_free(&packet);
        packet = NULL;
    }

    return 0;
}
