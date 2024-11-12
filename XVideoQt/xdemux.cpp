#include "xdemux.h"
extern "C"{
#include <libavformat/avformat.h>
}
#include<iostream>

// #define CheckError(err, str) (if(err != 0){std::cout<<str<< av_err2str(err) << std::endl; return nullptr;})


AVFormatContext* XDemux::Open(const char* url){
    AVFormatContext* c = nullptr;
    //打开流
    auto re = avformat_open_input(&c, url, nullptr, nullptr);
    // return CheckError(re, "avformat_open_input");
    if(re != 0){
        std::cout<<"avformat_open_input"<< av_err2str(re) << std::endl;
        return nullptr;
    }
    //获取媒体信息
    re = avformat_find_stream_info(c, nullptr);
    if(re != 0){
        std::cout<<"avformat_find_stream_info"<< av_err2str(re) << std::endl;
        return nullptr;
    }
    av_dump_format(c, 0, url, 0);//打印输入信息

    return c;
}

bool XDemux::Read(AVPacket *pkt){
    std::unique_lock<std::mutex> lock(mtx_);
    if(!c_)return false;
    auto re = av_read_frame(c_, pkt);
    if(re != 0){
        std::cout<<"av_read_frame"<< av_err2str(re) << std::endl;
        return false;
    }
    return true;
}

bool XDemux::Seek(long long pts, int stream_index){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_)return false;

    auto re = av_seek_frame(c_, stream_index, pts, AVSEEK_FLAG_FRAME|AVSEEK_FLAG_BACKWARD);
    if(re != 0){
        std::cout<<"av_seek_frame"<< av_err2str(re) << std::endl;
        return false;
    }
    return true;
}
