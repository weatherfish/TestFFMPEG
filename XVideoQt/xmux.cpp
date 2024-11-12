#include "xmux.h"
#include <iostream>
extern "C"{
#include <libavformat/avformat.h>
}

AVFormatContext* XMux::Open(const char* url){
    AVFormatContext* c = nullptr;
    //打开流
    auto re = avformat_alloc_output_context2(&c, nullptr, nullptr, url);
    // return CheckError(re, "avformat_open_input");
    if(re != 0){
        std::cout<<"avformat_open_input"<< av_err2str(re) << std::endl;
        return nullptr;
    }


    auto vs = avformat_new_stream(c, nullptr);//添加视频流
    vs->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

    auto as = avformat_new_stream(c, nullptr);//添加音频流
    as->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;

    //打开io
    re = avio_open(&c->pb, url, AVIO_FLAG_WRITE);
    if(re != 0){
        std::cout<<"avio_open"<< av_err2str(re) << std::endl;
        return nullptr;
    }

    return c;
}


bool XMux::WriteHead(){
    std::unique_lock<std::mutex> lock(mtx_);
    if(!c_)return false;
    auto re = avformat_write_header(c_, nullptr);
    if(re != 0){
        std::cout<<"avformat_write_header"<< av_err2str(re) << std::endl;
        return false;
    }

    //打印输出上下文
    av_dump_format(c_, 0, c_->url, 1);

    return true;
}

bool XMux::Write(AVPacket *pkt){
    std::unique_lock<std::mutex> lock(mtx_);
    if(!c_)return false;
    auto re = av_interleaved_write_frame(c_, pkt);//写入一帧数据，内部缓存排序dts，通过pkt=null可以写入缓存区
    if(re != 0){
        std::cout<<"av_interleaved_write_frame"<< av_err2str(re) << std::endl;
        return false;
    }

    return true;
}

bool XMux::WriteEnd(){
    std::unique_lock<std::mutex> lock(mtx_);
    if(!c_)return false;
    auto re = av_interleaved_write_frame(c_, nullptr);//写入缓冲区数据
    if(re != 0){
        std::cout<<"av_interleaved_write_frame"<< av_err2str(re) << std::endl;
        return false;
    }
    re = av_write_trailer(c_);
    if(re != 0){
        std::cout<<"av_write_trailer"<< av_err2str(re) << std::endl;
        return false;
    }

    return true;
}
