//
//  ReSample.c
//  TestFFMPEG
//
//  Created by Felix on 2024/6/12.
//

#include "ReSample.h"

// 全局变量声明
int resStatus;

void setStatus(int status){
    resStatus = status;
}

int getStatus(){
    return resStatus;
}

AVCodecContext* openCoder(){
    int ret = 0;
    char errors[1024] = {0,};
    
    const AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec) {
        fprintf(stderr, "编码器未找到\n");
        return NULL;
    }
    
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        fprintf(stderr, "无法分配音频编码器上下文\n");
        return NULL;
    }
    
    AVChannelLayout channelLayout;
    if (av_channel_layout_from_string(&channelLayout, "stereo") < 0) {
        fprintf(stderr, "无法从字符串获取声道布局\n");
        return NULL;
    }
    
    codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    codecContext->ch_layout = channelLayout;
    codecContext->sample_rate = 44100;
    codecContext->bit_rate = 0; // 设置为非零以使用固定比特率
    codecContext->profile = FF_PROFILE_AAC_HE_V2;
    
    // 打开编码器
    if ((ret = avcodec_open2(codecContext, codec, NULL)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "打开编码器失败, [%d] %s\n", ret, errors);
        return NULL;
    }
    
    return codecContext;
}

AVFormatContext* openDevice(const char *audioName){
    int ret = 0;
    char errors[1024] = {0,};

    AVFormatContext *fmtContext = NULL;
    AVDictionary *options = NULL;
    char *deviceName = ":1"; // 假设音频设备的名称为":1"

    // 假设audioName是一个有效的音频格式名称
    const AVInputFormat *iformat = av_find_input_format(audioName);

    // 尝试打开音频设备
    if((ret = avformat_open_input(&fmtContext, deviceName, iformat, &options)) < 0){
        av_strerror(ret, errors, 1024); // 获取错误信息
        fprintf(stderr, "打开音频设备失败, [%d]%s\n", ret, errors); // 打印错误信息
        return NULL; // 打开设备失败，返回NULL
    }
    return fmtContext; // 打开设备成功，返回格式上下文
}

AVFrame *createFrame(){
    AVFrame *frame = av_frame_alloc();
    if(!frame){
        fprintf(stderr, "错误, 内存不足\n");
        return NULL;
    }
    
    AVChannelLayout channelLayout;
    if (av_channel_layout_from_string(&channelLayout, "stereo") < 0) {
        fprintf(stderr, "无法从字符串获取声道布局\n");
        return NULL;
    }
    frame->nb_samples = 512;
    frame->format = AV_SAMPLE_FMT_S16;
    frame->ch_layout = channelLayout;

    if (av_frame_get_buffer(frame, 0) < 0) {
        fprintf(stderr, "在frame中分配缓冲区失败\n");
        av_frame_free(&frame);
        return NULL;
    }

    return frame;
}

SwrContext* init_swr() {
    SwrContext *swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "无法分配SwrContext\n");
        return NULL;
    }
    
    AVChannelLayout channelLayout;
    if (av_channel_layout_from_string(&channelLayout, "stereo") < 0) {
        fprintf(stderr, "无法从字符串获取声道布局\n");
        return NULL;
    }
    
    // 使用swr_alloc_set_opts2函数设置SwrContext
    int ret = swr_alloc_set_opts2(&swr_ctx,
                                  &channelLayout, AV_SAMPLE_FMT_S16, 44100,
                                  &channelLayout, AV_SAMPLE_FMT_FLT, 44100,
                                  0, NULL);
    if (ret < 0) {
        fprintf(stderr, "无法分配SwrContext\n");
        swr_free(&swr_ctx);
        return NULL;
    }

    // 初始化SwrContext
    if (swr_init(swr_ctx) < 0) {
        fprintf(stderr, "初始化SwrContext失败\n");
        swr_free(&swr_ctx);
        return NULL;
    }

    return swr_ctx;
}

void encodeAudio(AVCodecContext *codecContext, AVFrame *frame, AVPacket *packet, FILE* outFile){
    int ret = avcodec_send_frame(codecContext, frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext, packet);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if(ret < 0) {
            fprintf(stderr, "编码过程中出错\n");
            exit(1);
        }
        fwrite(packet->data, 1, packet->size, outFile);
        fflush(outFile);
    }
}

void allocData4Sampler(uint8_t ***src_data, int* src_linesize, uint8_t ***dst_data, int* dst_linesize){
    av_samples_alloc_array_and_samples(src_data, src_linesize, 2, 512, AV_SAMPLE_FMT_FLT, 0);
    av_samples_alloc_array_and_samples(dst_data, dst_linesize, 2, 512, AV_SAMPLE_FMT_S16, 0);
}

void deallocData4Sampler(uint8_t ***src_data, uint8_t ***dst_data){
    if(src_data){
        av_freep(&(*src_data)[0]);
        av_freep(src_data);
    }

    if(dst_data){
        av_freep(&(*dst_data)[0]);
        av_freep(dst_data);
    }
}

void readDataAndEncode(AVFormatContext* fmtContext, AVCodecContext * codecContext, SwrContext *swrContext,  FILE* outFile){
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;

    uint8_t **src_data = NULL;
    int src_linesize = 0;
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    AVPacket pkt;
    int ret = 0;

    frame = createFrame();
    if(!frame){
        goto __ERROR;
    }

    packet = av_packet_alloc();
    if(!packet){
        goto __ERROR;
    }

    allocData4Sampler(&src_data, &src_linesize, &dst_data, &dst_linesize);
     
    while ((ret = av_read_frame(fmtContext, &pkt)) == 0 && resStatus) {
        memcpy(src_data[0], pkt.data, pkt.size);
        swr_convert(swrContext, dst_data, 512, (const uint8_t * const *)src_data, 512);
        memcpy(frame->data[0], dst_data[0], dst_linesize);
        encodeAudio(codecContext, frame, packet, outFile);
        av_packet_unref(&pkt);
    }
    encodeAudio(codecContext, NULL, packet, outFile);

__ERROR:
    if(frame){
        av_frame_free(&frame);
    }
    if(packet){
        av_packet_free(&packet);
    }
    deallocData4Sampler(&src_data, &dst_data);
}

void recAudio(void){
    AVFormatContext *fmtContext = NULL;
    AVCodecContext* codecContext = NULL;
    SwrContext *swrContext = NULL;

    resStatus = 1;
    char *out = "audio.aac";
    FILE *outFile = fopen(out, "wb+");
    if(!outFile){
        fprintf(stderr, "无法打开文件\n");
        goto __ERROR;
    }

    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();

    fmtContext = openDevice("avfoundation");
    if(!fmtContext){
        fprintf(stderr, "打开设备失败\n");
        goto __ERROR;
    }

    codecContext = openCoder();
    if(!codecContext){
        fprintf(stderr, "打开编码器失败\n");
        goto __ERROR;
    }

    swrContext = init_swr();
    if(!swrContext){
        fprintf(stderr, "分配SwrContext失败\n");
        goto __ERROR;
    }

    readDataAndEncode(fmtContext, codecContext, swrContext, outFile);

__ERROR:
    if(swrContext){
        swr_free(&swrContext);
    }
    if(codecContext){
        avcodec_free_context(&codecContext);
    }
    if(fmtContext){
        avformat_close_input(&fmtContext);
    }
    if(outFile){
        fclose(outFile);
    }
    
    fprintf(stderr, "完成\n");
}
