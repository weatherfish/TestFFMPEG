#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>

static int select_best_sample_rate(const AVCodec *codec){
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates){
        return 44100;
    }
    p = codec->supported_samplerates;
    while (*p) {
        if(!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate)){
            best_samplerate = *p;
        }
        p++;
    }
    return best_samplerate;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sampleFmt){
    const enum AVSampleFormat *p= codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE)//AV_SAMPLE_FMT_NONE最后一项
    {
        if (*p == sampleFmt){
            return 1;
        }
        p++;
    }
    return 0;
}

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

int encodeAudio(int argc, char* argv[]){

    int ret = -1;
    char* dst;
    char* codecName = NULL;

    const AVCodec *codec;
    AVCodecContext *codecContext = NULL;

    av_log_set_level(AV_LOG_DEBUG);

    //1、输入参数
    if(argc < 2){
        av_log(NULL, AV_LOG_ERROR, "参数必须大于3\n");
        goto _ERROR;
    }

    dst = argv[1];
    // codecName = argv[1];

    //2 查找编码器  通过id  通过名称 （本次）
    // codec = avcodec_find_encoder_by_name("libfdk-aac"); //两个都可以，这个可以使用第三方的
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);  //这个使用的是ffmpeg内部的
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
    codecContext->bit_rate = 64000;
    // codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    codecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
    if(!check_sample_fmt(codec, codecContext->sample_fmt)){
        av_log(NULL, AV_LOG_ERROR, "编码器不支持sample format\n");
        goto _ERROR;
    }
    codecContext->sample_rate = select_best_sample_rate(codec);
    // av_channel_layout_copy(&codecContext->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
    av_channel_layout_copy(&codecContext->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_MONO);

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
    
    frame->nb_samples = codecContext->frame_size;
    // frame->format = codecContext->sample_fmt;
    // frame->format = AV_SAMPLE_FMT_S16;
    frame->format = AV_SAMPLE_FMT_FLTP;
    // av_channel_layout_copy(&frame->ch_layout, &codecContext->ch_layout);
    // av_channel_layout_copy(&frame->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
    av_channel_layout_copy(&frame->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_MONO );

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

    // uint16_t* samples;
    uint32_t* samples;
    //9 生成音频内容
    float t = 0;
    float tincr = 2*M_PI*440/codecContext->sample_rate;
   for (size_t i = 0; i < 200; i++) {
        ret = av_frame_make_writable(frame);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "无法分配空间 av_frame_make_writable\n");
            goto _ERROR;
        }
        // samples = (uint16_t*) frame->data[0];
        samples = (uint32_t*) frame->data[0];
        for (size_t j = 0; j < codecContext->frame_size; j++){
            samples[4*j] = (int)(sin(t) * 10000);
            for (size_t k = 1; k < codecContext->ch_layout.nb_channels; k++){
                samples[4*j + k] = samples[4*j];
            }
            t += tincr;
        }
        encode(codecContext, frame, packet, f);
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