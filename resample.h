#pragma once

#include <iostream>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

extern "C" {
    #include <stdio.h>
    #include <libavutil/avutil.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}


class ReSample
{
private:
    int resStatus;

public:
    ReSample(/* args */);
    ~ReSample();

    void setStatus(int status);
    void recVideo(void); 


private:

AVCodecContext*  openCoder(){
    const AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    AVCodecContext * codecContext = avcodec_alloc_context3(codec);
    AVChannelLayout channelLayout = AV_CHANNEL_LAYOUT_STEREO;
    
    codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    codecContext->ch_layout = channelLayout;
    codecContext->sample_rate = 441000;
    // codecContext->bit_rate = 64000;
    codecContext->bit_rate = 0;
    codecContext->profile = FF_PROFILE_AAC_HE_V2;

    if(avcodec_open2(codecContext, codec, NULL) <0){

    }
    return codecContext;
}

void openVideoEncoder(int width, int height, AVCodecContext **codecContext){
    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if(!codec){
        std::cout<<"Codec lib264x not found "<<std::endl;
        exit(0);
    }
    *codecContext = avcodec_alloc_context3(codec);
    if(!codecContext){
        std::cout<<"Cound not alloc  video codec "<<std::endl;
        exit(0);
    }
  
    (*codecContext)->profile = FF_PROFILE_H264_HIGH_444;
    (*codecContext)->level = 50; //level  5.0
   
    (*codecContext)->width = FRAME_WIDTH;
    (*codecContext)->height = FRAME_HEIGHT;
  
    (*codecContext)->gop_size = 250;
    (*codecContext)->keyint_min = 25; //最小i帧间距

    (*codecContext)->max_b_frames = 3; //最大B帧
    (*codecContext)->has_b_frames = 1;  

    (*codecContext)->refs = 3; //参考帧的数量

    (*codecContext)->pix_fmt = AV_PIX_FMT_YUV420P; //输入YUV格式


    (*codecContext)->bit_rate = 600 * 1000; //码率

    (*codecContext)->time_base = (AVRational){1, 25};///帧间隔，帧率的倒数

    (*codecContext)->framerate = (AVRational){25, 1}; ///帧率

    int ret = avcodec_open2((*codecContext),codec, NULL);
    if(ret < 0){
        std::cout<<"Can not open codec"<<std::endl;
        exit(1);
    }
}

//打开音频设备
AVFormatContext* openDevice(const char *deviceName){
    int ret = 0;
    char errors[1024] = {0,};

    AVFormatContext *fmtContext = NULL;
    AVDictionary *options = NULL;


    const AVInputFormat *iformat = av_find_input_format("AVFoundation");

    av_dict_set(&options, "video_size", "640x480", 0); 
    av_dict_set(&options, "framerate", "30", 0); 
    av_dict_set(&options, "pixel_format", "nv12", 0); 

    if((ret = avformat_open_input(&fmtContext, deviceName, iformat, &options)) < 0){
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return nullptr;
    } 
    return fmtContext;
}

AVFrame *createFrame(int width, int height){
    AVChannelLayout channelLayout = AV_CHANNEL_LAYOUT_STEREO;

    //音频输入
    AVFrame *frame = av_frame_alloc();
    if(!frame){
        std::cout<<"Error, no momery"<<std::endl;
        goto __ERROR;
    }

    //设置参数
    // frame->nb_samples = 512; //单通道一个音频帧每秒采样数
    frame->format = AV_PIX_FMT_YUV420P;
    frame->ch_layout = channelLayout;
    frame->width = width;
    frame->height = height;

    //申请Buffer
    // av_frame_get_buffer(frame, 0);  //音频
    av_frame_get_buffer(frame, 32); //视频 按照32位对齐
    if(!frame->buf[0]){
        std::cout<<"Failed to alloc buffer in frame"<<std::endl;
        goto __ERROR;
    }

    // 检查步幅是否正确设置
    if (frame->linesize[0] < frame->width) {
        fprintf(stderr, "Stride too small\n");
        return nullptr;
    }

    return frame;

__ERROR:
    if(frame){
        av_frame_free(&frame);
    }
    return NULL;
}

SwrContext *initSwr(){
    AVChannelLayout channelLayout = AV_CHANNEL_LAYOUT_STEREO;

    // 分配 SwrContext
    SwrContext *swrContext = swr_alloc();

    // 使用正确的采样率设置 SwrContext 参数
    swr_alloc_set_opts2(&swrContext, &channelLayout, AV_SAMPLE_FMT_S16, 44100, 
                        &channelLayout, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);


    if(!swrContext){

    }

    if(swr_init(swrContext) < 0 ){

    }
    return swrContext;
}

void encodeData(AVCodecContext *codecContext, AVFrame *frame, AVPacket *packet, FILE* outFile){
     int ret = avcodec_send_frame(codecContext,frame);
    while (ret >= 0)
    {
        //获取编码后的音频数据
        ret = avcodec_receive_packet(codecContext, packet);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return;
        }else if(ret < 0){
            exit(-1); 
        }
        // fwrite(packet->data, 1, packet->size, outFile);
        //分辨率 X 1.5
        float count = 1;
        if(frame->format == AVPixelFormat::AV_PIX_FMT_YUV420P){
            count = 1.5;
        }else  if(frame->format == AVPixelFormat::AV_PIX_FMT_YUV422P){
            count = 2;
        } if(frame->format == AVPixelFormat::AV_PIX_FMT_YUV444P){
            count = 3;
        }
        
        size_t size = frame->width * frame->height * count;

        std::cout<<"Out file format = "<< frame->format <<" 分辨率 = "<< frame->width <<"x"<<frame->height<<std::endl;

        fwrite(packet->data, 1, size, outFile);
        fflush(outFile); 
    }
}

void allocData4Sampler(uint8_t ***src_data, int* src_linesize, uint8_t ***dst_data, int* dst_linesize){
     //输入缓存区
    av_samples_alloc_array_and_samples(src_data, src_linesize, 2, 512, AV_SAMPLE_FMT_FLT, 0);
    //输出缓存区
    av_samples_alloc_array_and_samples(dst_data, dst_linesize, 2, 512, AV_SAMPLE_FMT_S16, 0); 
}

void dellocData4Sampler(uint8_t ***src_data, uint8_t ***dst_data){
    if(src_data){
        av_freep(&src_data[0]);
        av_freep(src_data);
    }

    if(dst_data){
        av_freep(&dst_data[0]);
        av_freep(dst_data);
    }
}

void readDataAndEncode(AVFormatContext* fmtContext, AVCodecContext * codecContext, SwrContext *swrContext,  FILE* outFile){
    
    AVPacket * packet = nullptr;
    AVFrame *frame = nullptr;

    uint8_t **src_data = NULL;
    int src_linesize = 0;
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    AVPacket pkt;
    int ret = 0;

    frame = createFrame(FRAME_WIDTH, FRAME_HEIGHT);
    if(!frame){
        std::cout<<"createFrame error"<<std::endl;
        goto __ERROR;
    }

    packet = av_packet_alloc();
    if(!packet){
        std::cout<<"alloc packet error"<<std::endl;
        goto __ERROR;
    }

    // allocData4Sampler(&src_data, &src_linesize, &dst_data, &dst_linesize);
     
    while ((ret = av_read_frame(fmtContext, &pkt)) == 0 && resStatus)
    {
        av_log(NULL, AV_LOG_INFO, "Packaeges Size is %d(%p)]\n", pkt.size, pkt.data);

        // fwrite(pkt.data, 1, 460800, outFile);

        memcpy(frame->data[0], (void*)pkt.data[0], 307200); //y数据

        for (size_t i = 0; i < 307200/4; i++)
        {
            frame->data[1][i] = pkt.data[307200+i];
            frame->data[2][i] = pkt.data[307200+i+1];
        }

        fwrite(frame->data[0], 1, 307200, outFile);
        fwrite(frame->data[1], 1, 30720/4, outFile);
        fwrite(frame->data[2], 1, 307200/4, outFile);
        fflush(outFile);

        

        /**
        memcpy(src_data[0], pkt.data, pkt.size);
        //重采样
        swr_convert(swrContext, dst_data, 512, (uint8_t * const *)src_data, 512);

        memcpy(frame->data[0], dst_data[0], dst_linesize); //重采样数据拷贝到frame

        encodeData(codecContext, frame, packet, outFile);
        */
        av_packet_unref(&pkt);
    }
    //强制缓冲区数据进行编码 
    // encodeData(codecContext, NULL, packet, outFile);

__ERROR:
    if(frame){
        av_frame_free(&frame);
    }
    if(packet){
        av_packet_free(&packet);
    }
     
    // dellocData4Sampler(&src_data, &dst_data);
}

};


