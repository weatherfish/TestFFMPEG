#include "xvideo_view.h"
#include "xsdl.h"
#include <thread>
#include<iostream>
extern "C"{
#include <libavcodec/avcodec.h>
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
        return Draw(frame->data[0], frame->linesize[0]);
         break;
    default:
        break;
    }

    return true;
}

void XVideoView::MSleep(unsigned int ms){
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ms; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto end = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() >= ms)break;
    }
}
