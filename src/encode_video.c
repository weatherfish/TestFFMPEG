#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>

static int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *f){
    int ret = -1;
    ret = avcodec_send_frame(ctx, frame);
    if(ret<0){
        av_log(NULL, AV_LOG_ERROR, "发送frame失败\n");
        goto _END;
    }

    while (ret >= 0){
        ret =avcodec_receive_packet(ctx, pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){
            return -1;//返回-1表示失败，需要退出程序
        }

        fwrite(pkt->data, 1, pkt->size, f);
        av_packet_unref(pkt);
    }
_END:
    return 0;
}

int encodeVideo(int argc, char* argv[]){

    int ret = -1;
    char* dst;
    char* codecName = NULL;

    const AVCodec *codec;
    AVCodecContext *codecContext = NULL;

    av_log_set_level(AV_LOG_DEBUG);

    //1、输入参数
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "参数必须大于3\n");
        goto _ERROR;
    }
    dst = argv[1];
    codecName = argv[1];

    //2 查找编码器  通过id  通过名称 （本次）
    codec = avcodec_find_encoder_by_name(codecName);
    if (!codec){
        av_log(NULL, AV_LOG_ERROR, "无法为 %s 找到Codec\n", codecName);
        goto _ERROR;
    }

    //3 创建编码器上下文
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext){
        av_log(NULL, AV_LOG_ERROR, "内存不够，调用avcodec_alloc_context3失败\n");
        goto _ERROR;
    }

    //4 设置编码器参数
    codecContext->width = 640;
    codecContext->height = 480;
    codecContext->bit_rate = 500000;

    codecContext->time_base = (AVRational){1, 25};
    codecContext->framerate = (AVRational){25, 1};  //帧率

    codecContext->gop_size = 10;
    codecContext->max_b_frames = 1; //最大多少B帧
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if(codecContext->codec_id == AV_CODEC_ID_H264){//预设置参数
        av_opt_set(codecContext->priv_data, "preset", "slow", 0); //slow清晰度更高
    }

    //5 绑定编码器和上下文
    ret = avcodec_open2(codecContext, codec, NULL);
    if ((ret < 0))
    {
        av_log(codecContext, AV_LOG_ERROR, "无法打开codec: %s\n", av_err2str(ret));
        goto _ERROR;
    }

    //6 创建输出文件
    FILE *f =fopen(dst, "wb");
    if(!f){
        av_log(NULL, AV_LOG_ERROR, "打开文件失败: %s\n", dst);
        goto _ERROR;
    }

    //7 创建AVFrame  元数据
    AVFrame* frame = av_frame_alloc();
    if(!frame){
         av_log(NULL, AV_LOG_ERROR, "内存不足 av_frame_alloc\n");
        goto _ERROR;
    }

    frame->width = codecContext->width;
    frame->height = codecContext->height;
    frame->format = codecContext->pix_fmt;

    ret = av_frame_get_buffer(frame, 0);
    if(ret < 0 ){
        av_log(NULL, AV_LOG_ERROR, "无法分配AVFrame\n");
        goto _ERROR;
    }

    //8 创建AVPacket  编码后数据
    AVPacket *packet = av_packet_alloc();
    if(!packet){
        av_log(NULL, AV_LOG_ERROR, "内存不足 av_packet_alloc\n");
        goto _ERROR;
    }

    //9 生成视频内容
    for (size_t i = 0; i < 25; i++){
        ret = av_frame_make_writable(frame);//确保date域是否被锁定
        if(ret < 0 ){
            break;
        }
        //Y分量
        for (size_t y = 0; y < codecContext->height; y++) {
            for (size_t x = 0; x < codecContext->width; x++){
                frame->data[0][y*frame->linesize[0]+x] = x + y + i * 3;
            }
        }
        //uv分量
        for (size_t y = 0; y < codecContext->height; y++){
            for (size_t x = 0; x < codecContext->width; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2 ;//128黑色
                frame->data[2][y * frame->linesize[2] + x] = 64 + y + i * 5 ;//128黑色
            }
        }
        
        frame->pts = i;
        //10 编码
        ret = encode(codecContext, frame, packet, f);
        if(ret == -1){
            goto _ERROR;
        }
    }
    
    //强制输出缓存
    encode(codecContext, NULL, packet, f);

_ERROR:
    if(codecContext){
        avcodec_free_context(&codecContext);
    }

    if(frame){
        av_frame_free(&frame);
    }

    if(packet){
        av_packet_free(&packet);
    }

    if(f){
        fclose(f);
    }
    return 0; 
}