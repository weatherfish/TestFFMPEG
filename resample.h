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