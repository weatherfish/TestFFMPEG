#include "resample.h"
#include <iostream>

ReSample::ReSample(/* args */)
{
}

ReSample::~ReSample()
{
}

void ReSample::setStatus(int status){
    this->resStatus = status; 
}

void ReSample::recAudio(void){

    AVFormatContext *fmtContext = nullptr;
    AVCodecContext*  codecContext = nullptr;
    SwrContext *swrContext = nullptr;

    int ret = 0;
    char errors[1024] = {0,};
    char *deviceName = ":0";
    resStatus = 1;
    // char *out = "audio.pcm";
    char *out = "audio.aac"; 
    FILE *outFile = fopen(out, "wb+");
    if(!outFile){
        std::cout<<"Failed to open file"<<std::endl;
        goto __ERROR; 
    }

    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();

    //打开设备
    fmtContext = openDevice("avfoundation");
    if(!fmtContext){
        std::cout<<"Failed to open file"<<std::endl;
        goto __ERROR; 
    }

    //打开编码器上下文
    codecContext = openCoder();
    if(!codecContext){
        std::cout<<"Failed to openCoder"<<std::endl;
        return;
    } 

    swrContext = initSwr(); 
    if(!swrContext){
        std::cout<<"Failed to alloc SwrContext"<<std::endl;
        return;
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
    
    av_log(NULL, AV_LOG_DEBUG, "finish \n"); 
    
}
