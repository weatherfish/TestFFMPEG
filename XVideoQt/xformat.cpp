#include "xformat.h"
#include <iostream>
extern "C"{
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
}


#define CheckError(err, str) (if(err !=0) std::cout<<str<< av_err2str(err) <<std::endl);


static int TimeoutCallback(void *params){
    auto fs = (XFormat*)params;
    std::cout<<"TimeoutCallback"<<std::endl;
    return 0; //正常阻塞
}

void XFormat::setC(AVFormatContext* c){
    std::unique_lock<std::mutex>lock(mtx_);
    if(c_){//清理
        if(c_->oformat){
            if(c_->pb){
                avio_closep(&c_->pb);
                 avformat_free_context(c_);
            }
        }else if(c_->iformat){
            avformat_close_input(&c_);
        }else{
            avformat_free_context(c_);
        }


    }
    c_ = c;

    if(!c_)return;

    audio_index_ = -1;
    video_index_ = -1;

    if(time_out_ms_ > 0){
        AVIOInterruptCB cb = {TimeoutCallback, this};
        c_->interrupt_callback = cb;
    }

    for (int i = 0; i < c_->nb_streams; ++i) {
        if(c_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index_ = i;
            audio_time_base_.den = c_->streams[i]->time_base.den;
            audio_time_base_.num = c_->streams[i]->time_base.num;
        }else if(c_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index_ = i;
            video_time_base_.den = c_->streams[i]->time_base.den;
            video_time_base_ .num = c_->streams[i]->time_base.num;
        }
    }
}

bool XFormat::CopyParams(int stream_index, AVCodecParameters* dst){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;
    if(stream_index < 0  || stream_index > c_->nb_streams)return false;

    auto re = avcodec_parameters_copy(dst, c_->streams[stream_index]->codecpar);
    if(re < 0){
        std::cout<<"avcodec_parameters_copy"<<av_err2str(re)<<std::endl;
        return false;
    }
    return true;
}

bool XFormat::RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base){
    std::unique_lock<std::mutex>lock(mtx_);
    if(!c_) return false;

    auto out_stream = c_->streams[pkt->stream_index];
    AVRational in_time_base;
    in_time_base.den = time_base.den;
    in_time_base.num = time_base.num;

    pkt->pts = av_rescale_q_rnd(pkt->pts - offset_pts, in_time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF| AV_ROUND_PASS_MINMAX));
    pkt->dts = av_rescale_q_rnd(pkt->dts - offset_pts, in_time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF| AV_ROUND_PASS_MINMAX));
    pkt->duration = av_rescale_q(pkt->duration, in_time_base, out_stream->time_base);
    pkt->pos = -1;

    return true;
}



void XFormat::setTimeOutMs(int ms){
    std::unique_lock<std::mutex>lock(mtx_);
    this->time_out_ms_ = ms;
    if(c_){ //设置回调喊出，处理超时退出
        AVIOInterruptCB cb = {TimeoutCallback, this};
        c_->interrupt_callback = cb ;
    }
}
