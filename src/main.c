#include <unistd.h>
#include "get_info.h"
#include "extra_audio.h"

int main(int argc, char* argv[]){
    // AVGeneralMediaInfo *info = new AVGeneralMediaInfo();
    // if(info){
    //     get_avgeneral_mediainfo(info, "1.mp4");
        
    //     delete info;
    //     info = NULL;
    // }

    extra_audio(argc, argv);

    return 0; // Successful execution
}
