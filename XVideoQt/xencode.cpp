#include "xencode.h"
#include <iostream>

extern "C" {
#include<libavcodec/avcodec.h>
#include<libavutil/opt.h>
}

AVPacket* XEncode::Encode(const AVFrame* frame){
    if(!frame) return nullptr;
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_ ) return nullptr;

    auto re = avcodec_send_frame(c_, frame); //发送给编码线程
    if(re != 0){
        std::cout<<"avcodec_send_frame error :"<<av_err2str(re)<<std::endl;
        return nullptr ;
    }

    auto pkt = av_packet_alloc();

    re = avcodec_receive_packet(c_, pkt); //接收编码线程数据
    if(re == 0){
        return pkt;
    }
    av_packet_free(&pkt);
    if(re == AVERROR(EAGAIN) || re == AVERROR_EOF){

    }else if(re < 0){
        std::cout<<"avcodec_receive_packet error :"<<av_err2str(re)<<std::endl;
    }

    return nullptr;
}

std::vector<AVPacket*> XEncode::End(){
    std::vector<AVPacket*> res;
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_ ) return res;

    auto re = avcodec_send_frame(c_, nullptr); // 获取缓冲数据
    if(re != 0) return res;
    while (re >= 0) {
        auto pkt = av_packet_alloc();
        re = avcodec_receive_packet(c_, pkt); //接收编码线程数据
        if(re != 0){
            av_packet_free(&pkt);
            break;
        }
        res.push_back(pkt);
    }
    return res;
}
