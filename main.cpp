#include <iostream>
#include <thread> 
#include <unistd.h>

#include "resample.h"


void stopVideo(ReSample* sample){
    sleep(5);
    sample->setStatus(0);
}

int main(int argc, char** argv){
    ReSample *sample = new ReSample();
    
    std::thread t(stopVideo, std::ref(sample));
    sample->recVideo();
    t.join();
    free(sample);

    return 0; // Successful execution
}
