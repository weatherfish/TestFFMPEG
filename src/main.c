#include <unistd.h>
#include "get_info.h"
#include "extra.h"

int main(int argc, char* argv[]){
    // AVGeneralMediaInfo *info = new AVGeneralMediaInfo();
    // if(info){
    //     get_avgeneral_mediainfo(info, "1.mp4");
        
    //     delete info;
    //     info = NULL;
    // }

    // extra_audio(argc, argv);
    // extra_video(argc, argv);
    // remux(argc, argv);
    // cut(argc, argv);
    // encodeVideo(argc, argv);
    // encodeAudio(argc, argv);
    // encodeAudio2(argc, argv);
    gen_pic(argc, argv);
    // gen_pic2(argc, argv);

    return 0; // Successful execution
}
