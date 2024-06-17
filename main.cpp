#include <iostream>
#include <thread> 
#include <unistd.h>

#include "resample.h"


void stopVideo(ReSample* sample){
    sleep(30);
    // sample::setStatus(0);
}

int main(int argc, char** argv){
    // ReSample *sample = new ReSample("sample.mp4", "video.yuv");
    // ReSample *sample = new ReSample("0", "video.yuv");
    
    // std::thread t(stopVideo, std::ref(sample));
    // sample->startRecording();
    // t.join();
    // free(sample);

    AVGeneralMediaInfo *info = new AVGeneralMediaInfo();
    if(info){
        get_avgeneral_mediainfo(info, "ample.mp4");

        
        delete info;
        info = NULL;
    }

    return 0; // Successful execution
}
