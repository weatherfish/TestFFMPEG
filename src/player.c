// #include <libavutil/avutil.h>
// #include <libavcodec/avcodec.h>
// #include <libswresample/swresample.h>
// #include <SDL.h>

// static SDL_Window *win = NULL;
// static SDL_Renderer *renderer = NULL;

// static int default_width = 640;
// static int default_height = 480;

// typedef struct _VideoState {
//     AVCodecContext *aCtx;
//     AVCodecContext *vCtx;
//     SwrContext *swr_ctx;

//     uint8_t *audio_buf;
//     uint audio_buf_size;
//     int audio_buf_index;

//     AVPacket *apkt;
//     AVPacket *vpkt;
//     AVFrame *aframe;
//     AVFrame *vframe;
//     SDL_Texture *texture;

//     int audio_index;
//     int video_index;
//     char* filename;
//     int ytop;
//     int xleft;

//     // PacketQueue audioQueue;
// } VideoState;

// static stream_open(const char* filename){
//     VideoState *vs = av_mallocz(sizeof(VideoState));
//     if(!vs){
//         av_log(NULL, AV_LOG_FATAL, "No memory to create VideoState\n");
//         return NULL;
//     } 
//     vs->audio_index = vs->video_index = -1;
//     vs->filename = av_strdup(filename);
//     if(!vs->filename){
//         goto __ERROR;
//     }
    
//     vs->ytop = 0;
//     vs->xleft = 0;


// __ERROR:


// }

// int player(int argc, char* argv[]){
//     int ret = 0;
//     int flag = 0;

//     VideoState *vs;

//     av_log_set_level(AV_LOG_INFO);

//     // 1. 判断输入参数
//     if (argc < 2) {
//         av_log(NULL, AV_LOG_FATAL, "Usage: Command <flie>\n");
//         exit(-1);
//     }

//     const char* input_filename = argv[1];

//     flag = SDL_INIT_VIDEO|SDL_INIT_AUDIO |SDL_INIT_TIMER;

//     // 2. 初始化SDL，并创建窗口和Render
//     if (SDL_Init(flag)) {
//         av_log(NULL, AV_LOG_FATAL, "无法初始化 SDL: %s\n", SDL_GetError());
//          exit(-1);
//     }

//     win = SDL_CreateWindow("SimplePlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//                            default_width, default_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
//     if (win) {
//         renderer = SDL_CreateRenderer(win, -1, 0);
//     }

//     if (!win || !renderer) {
//         av_log(NULL, AV_LOG_FATAL, "无法创建Window 或者 renderer: %s\n", SDL_GetError());
//         do_exit(NULL);
//     }

//     vs = stream_open(input_filename);
//     if (!vs) {
//         av_log(NULL, AV_LOG_FATAL, "无法创建VideoState\n");
//         do_exit(NULL);
//     }

//     sdl_event_loop(vs); 

//     return ret;
// }