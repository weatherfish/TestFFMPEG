#include "xcodec.h"
#include <iostream>

extern "C" {
#include<libavcodec/avcodec.h>
#include<libavutil/opt.h>
}


AVCodecContext* XCodec::Create(int codeId, bool isEncode){
    //查找编码器
    const AVCodec *codec = nullptr;
    if(isEncode){
        codec = avcodec_find_encoder((AVCodecID)codeId);
    }else{
        codec = avcodec_find_decoder((AVCodecID)codeId);
    }

    if(!codec){
        std::cerr<<"find codec encoder failed: "<<codeId<<std::endl;
        return nullptr;
    }

    auto c = avcodec_alloc_context3(codec);
    if(!c){
        std::cerr<<"create context failed: "<<std::endl;
        return nullptr;
    }

    //设置默认参数
    c->time_base = {1, 25};
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->thread_count = 16;

    return c;
}

void XCodec::setContext(AVCodecContext* c){
    std::unique_lock<std::mutex>lock(mtx_);
    if(c_){
        avcodec_free_context(&c_);
    }
    this->c_ = c;
}

bool XCodec::SetOpt(const char* key, const char* val){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    auto  re = av_opt_set(c_, key, val, 0);
    if(re != 0){
        std::cout<<"av_opt_set error :"<<av_err2str(re)<<std::endl;
        return false;
    }

    return true;
}

bool XCodec::SetOpt(const char* key, int val){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    auto  re = av_opt_set_int(c_, key, val, 0);
    if(re != 0){
        std::cout<<"av_opt_set error :"<<av_err2str(re)<<std::endl;
        return false;
    }

    return true;
}

bool XCodec::OpenEncoder(){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    auto re = avcodec_open2(c_, nullptr, nullptr);
    if(re != 0){
        std::cout<<"avcodec_open2 error :"<<av_err2str(re)<<std::endl;
        return false;
    }

    return true;
}

AVFrame* XCodec::CreateFrame(){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_ ) return nullptr;

    auto frame = av_frame_alloc();
    frame->width = c_->width;
    frame->height = c_->height;
    frame->format = c_->pix_fmt;
    auto re = av_frame_get_buffer(frame, 0);
    if(re != 0){
        av_frame_free(&frame);
        std::cout<<"av_frame_get_buffer error :"<<av_err2str(re)<<std::endl;
        return nullptr ;
    }
    return frame;
}
