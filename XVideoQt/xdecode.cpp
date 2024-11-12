#include "xdecode.h"
#include <iostream>

extern "C" {
#include<libavcodec/avcodec.h>
#include<libavutil/opt.h>
#include <libavutil/hwcontext.h>
}


bool XDecode::Send(const AVPacket *pkt){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    auto re = avcodec_send_packet(c_, pkt);
    if(re != 0)return false;

    return true;
}
bool XDecode::Recieve(AVFrame *frame){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;
    auto f = frame;
    if(c_->hw_device_ctx){ //硬件加速
        f = av_frame_alloc();
    }

    auto re = avcodec_receive_frame(c_, f);
    if(re == 0){
        if(c_->hw_device_ctx){
            re = av_hwframe_transfer_data(frame, f, 0); //转内存数据
            av_frame_free(&f);
            if(re  != 0){
                std::cout<<"av_hwframe_transfer_data error :"<<av_err2str(re)<<std::endl;
                return false;
            }
        }
        return true;
    }

    if(c_->hw_device_ctx)av_frame_free(&f);
    return false;
}

std::vector<AVFrame*> XDecode::End(){
    std::vector<AVFrame*> res;

    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return res;

    auto re = avcodec_send_packet(c_, nullptr);
    while (re >= 0) {
        auto frame = av_frame_alloc();
        re = avcodec_receive_frame(c_, frame);
        if(re < 0){
            av_frame_free(&frame);
            break;
        }
        res.push_back(frame);
    }
    return res;
}

bool XDecode::initHW(int type){

    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    AVBufferRef *hwDeviceCtx = nullptr; //硬件加速上下文
    auto re = av_hwdevice_ctx_create(&hwDeviceCtx, (AVHWDeviceType)type, nullptr, nullptr, 0);
    if(re != 0){
        std::cout<<"av_hwdevice_ctx_create error :"<<av_err2str(re)<<std::endl;
        return false;
    }
    c_->hw_device_ctx = hwDeviceCtx;

    std::cout<<"硬件加速:"<<type<<std::endl;

    return true;
}
