#include "xvideo_view.h"
#include "xsdl.h"
#include<iostream>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

XVideoView::XVideoView() {}

XVideoView* XVideoView::create(RenderType type){
    switch (type) {
    case SDL:
        return new XSDL();
        break;
    default:
        break;
    }
    return nullptr;
}

bool XVideoView::DrawFrame(const AVFrame *frame){
    if(frame == nullptr || frame->data[0] == nullptr) return false;

    count_++;
    auto end = std::chrono::high_resolution_clock::now();
    if(beginMs.time_since_epoch().count() == 0){
        beginMs = std::chrono::high_resolution_clock::now();
    } else if(std::chrono::duration_cast<std::chrono::milliseconds>(end - beginMs).count() >= 1000){  //1s计算一次
        renderFps_ = count_;
        count_ = 0;
        beginMs = std::chrono::high_resolution_clock::now();
    }

    switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
        return Draw(frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
        break;
    case AV_PIX_FMT_ARGB:
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_ABGR:
        return Draw(frame->data[0], frame->linesize[0]);
         break;
    default:
        break;
    }

    return true;
}

bool XVideoView::Open(std::string filepath){
    if(ifs_.is_open())ifs_.close();
    ifs_.open(filepath, std::ios::binary);
    return ifs_.is_open();
}

AVFrame* XVideoView::Read(){
    if(width_ <=0 || height_ <= 0 || !ifs_) return nullptr;
    //参数变化，释放空间
    if(frame_){
        if(frame_->width != width_ || frame_->height != height_ || frame_->format != format_){
             //释放对象 以及 buffer引用计数减一
            av_frame_free(&frame_);
        }
    }
    if(!frame_){
        frame_ = av_frame_alloc();
        frame_->width = width_;
        frame_->height = height_;
        frame_->format = format_;
        if(frame_->format == AV_PIX_FMT_YUV420P){
            frame_->linesize[0] = width_;      //Y
            frame_->linesize[1] = width_ / 2;  //U
            frame_->linesize[2] = width_ / 2;  //V
        }else{
            frame_->linesize[0] = width_ * 4;
        }
        auto re = av_frame_get_buffer(frame_, 0);
        if(re != 0){
            std::cout<<av_err2str(re)<<std::endl;
            av_frame_free(&frame_);
            return nullptr;
        }
    }

    if(!frame_){
        return nullptr;
    }

    //读取一帧数据
    if(frame_->format == AV_PIX_FMT_YUV420P){
        ifs_.read((char*)frame_->data[0], frame_->linesize[0] * frame_->height); //Y
        ifs_.read((char*)frame_->data[1], frame_->linesize[1] * frame_->height / 2); //U
        ifs_.read((char*)frame_->data[2], frame_->linesize[2] * frame_->height / 2); //V
    }else{
        ifs_.read((char*)frame_->data[0], frame_->linesize[0] * frame_->height);
    }
    if(ifs_.gcount() == 0)return nullptr;

    return frame_;
}
