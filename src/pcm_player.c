#include <SDL.h>

#define BLOCK_SIZE  4096 * 1024
static size_t buffer_len = 0;
static Uint8* audio_buf  = NULL;
static Uint8* audio_pos = NULL;

void read_audio_data(void* udata, Uint8 *stream, int len){
    if(buffer_len == 0){
        return;
    }
    SDL_memset(stream, 0, len);
    len = (len < buffer_len)? len: buffer_len;
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME); //最大音量

    audio_pos += len;
    buffer_len -= len;
}

int pcm_player(int argc, char* argv[]){
    int ret = -1;
    char* path = "1.pcm";

    if(SDL_Init(SDL_INIT_AUDIO)){
        SDL_Log("Failed to initial!");
        return ret;
    }

    FILE* audioFd = fopen(path, "r");
    if(!audioFd){
        SDL_Log("Failed to open pcm file!");
        goto __FAIL;
    }

    Uint8* audio_buf = (Uint8*)malloc(BLOCK_SIZE);
    if(!audio_buf){
        SDL_Log("Failed to malloc audio_buf");
        goto __FAIL;
    }

    //打开设备
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.callback = read_audio_data;
    spec.userdata = NULL;

    if(SDL_OpenAudio(&spec, NULL)){
         SDL_Log("Failed to SDL_OpenAudio ");
        goto __FAIL;
    }

    //播放启动
    SDL_PauseAudio(0);// 0是播放，1 是暂停

    do{
        buffer_len = fread(audio_buf, 1, BLOCK_SIZE, audioFd);
        audio_pos = audio_buf;
        while (audio_pos < (audio_buf + buffer_len)){ //buffer里面还有数据
            SDL_Delay(1);
        }
    } while (buffer_len != 0);

    SDL_CloseAudio();

    ret = 0;

__FAIL:
    if(audio_buf){
        free(audio_buf);
    }
    if(audioFd){
        fclose(audioFd);
    }
    SDL_Quit();
    return ret;
}
