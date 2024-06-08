#pragma once

#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>


class resample
{
private:
    int resStatus;
    
public:
    resample(/* args */);
    ~resample();

    void setStatus(int status);

    void recAudio(void);


};


