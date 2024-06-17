// resample.h
#pragma once

#include <iostream>
#include <string>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}

typedef struct __AVGeneralMediaInfo{
    char filepath[1024];   //文件路径
    int64_t duration;   //时长 微秒  1000000
    int64_t totalBitrate;   //总码率
    int videoStreamIndex;   //视频流索引
    int audioStreamIndex;   //音频流索引

    char videoCodecName[256];   //视频编码器名
    int width;
    int height;
    double frameRate;  //帧率

    char audioCodecName[256];   //音频编码器名
    int sampleRate; //采样率
    int channels;   //声道数
} AVGeneralMediaInfo;

void get_avgeneral_mediainfo(AVGeneralMediaInfo *info, const char* filepath){
    int ret = -1;
    if(info == NULL || filepath == NULL){
        return;
    }
    AVFormatContext *formatContext = NULL;

    ret = avformat_open_input(&formatContext, filepath, NULL, NULL);
    if( ret<0 ){
        printf("Error open %s\n", filepath);
        return;
    }

    ret = avformat_find_stream_info(formatContext, NULL);
    if( ret<0 ){
        printf("Error find stream info %s\n", filepath);
        return;
    }

    av_dump_format(formatContext, 0, filepath, 0);

    info->duration = formatContext->duration;
    info->totalBitrate = formatContext->bit_rate;


    avformat_close_input(&formatContext);

}

class ReSample {
public:
    ReSample(const std::string& deviceName, const std::string& outputFilePath);
    ~ReSample();

    void startRecording();

private:
    bool openDevice();
    bool initVideoEncoder();
    AVFrame* allocFrame(int width, int height);
    void readAndEncodeData();
    void encodeData(AVFrame* frame);
    void cleanup();

    std::string deviceName;
    std::string outputFilePath;

    AVFormatContext* fmtContext = nullptr;
    AVCodecContext* videoCodecContext = nullptr;
    AVFrame* frame = nullptr;
    FILE* outFile = nullptr;

    const int frameWidth = 640;
    const int frameHeight = 480;
};