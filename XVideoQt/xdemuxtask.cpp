#include "xdemuxtask.h"
extern "C"{
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
}

void XDemuxTask::Main(){
    AVPacket pkt;

    while(!is_exit_){
        if(!demux_.Read(&pkt)){
            //读取失败
            std::cout<<"-"<<std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool XDemuxTask::Open(std::string url, int timeout){
    auto c = demux_.Open(url.c_str());
    if(!c)return false;
    demux_.setC(c);
    demux_.setTimeOutMs(timeout);
    return true;
}
