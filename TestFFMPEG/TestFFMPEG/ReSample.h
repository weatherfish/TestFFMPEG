//
//  ReSample.h
//  TestFFMPEG
//
//  Created by Felix on 2024/6/12.
//

#ifndef ReSample_h
#define ReSample_h

#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

int resStatus;

void setStatus(int status);

int getStatus();

void recAudio(void);




#endif /* ReSample_h */
