// #include <libavutil/avutil.h>
// #include <libavformat/avformat.h>
// #include <libavcodec/avcodec.h>
// #include <libswresample/swresample.h>
// #include <SDL.h>

// #include "queue.hpp"

// #define SDL_AUDIO_BUFFER_SIZE 1024

// static SDL_Window *win = NULL;
// static SDL_Renderer *renderer = NULL;

// static int default_width = 640;
// static int default_height = 480;

// typedef struct _VideoState {

//     uint8_t *audio_buf;
//     uint audio_buf_size;
//     int audio_buf_index;

//     AVFrame *audio_frame;
//     AVFrame *video_frame;
//     SDL_Texture *texture;

//     int audio_index;
//     int video_index;
//     char* filename;
//     int ytop;
//     int xleft;

//     PacketQueue audioq;
//     PacketQueue videoq;

//     PacketQueue pictq;

//     int read_tid;
//     int decode_tid;

//     SwrContext *audio_swr_ctx;
//     AVFormatContext *ic;
//     AVStream *audio_st;
//     AVStream *video_st;
//     AVCodecContext *audio_ctx;
//     AVCodecContext *video_ctx;

//     AVPacket *audio_pkt;
//     AVPacket *video_pkt;

//     double frame_timer;
//     double frame_last_delay;
//     double video_current_pts_time;

//     double audio_clock;
//     double video_clock;

//     int quit;
// } VideoState;

// double synchronize_video(VideoState *vs, AVFrame *src_frame, double pts){
//     double frame_delay;
//     if(pts != 0){
//         //if we have pts,set video clock to it
//         vs->video_clock = pts;
//     }else{
//         //if we are not given a pts, set it to the clock
//         pts = vs->video_clock;
//     }
//     //update the video clock
//     frame_delay = av_q2d(vs->video_ctx->time_base);
//     //if we are repeating a frame, adjust clock accordingly
//     frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
//     vs->video_clock +=frame_delay;
//     return pts;
// }

// static int queue_picture(VideoState *vs, AVFrame *src_frame, double pts, double duration, int64_t pos){
//     AVFrame *vp;

// #if defined(DEBUG_SYNC)
//     printf("frame type = %c pts = %0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
// #endif

//     if(!(vp = frame_queue_peek_writable(&vs->pictq)))
//         return -1;
    
//     vp->sar = src_frame->sample_aspect_ratio;
//     vp->width = src_frame->width;
//     vp->height = src_frame->height;
//     vp->format = src_frame->format;

//     vp->pts = pts;
//     vp->duration = duration;
//     vp->pos = pos;

//     av_frame_move_ref(vp->frame, src_frame);

//     frame_queue_push(&vs->pictq);

//     return 0;
// }

// int decode_thread(void *arg){
//     int ret = -1;
//     double pts;
//     double duration;

//     VideoState *vs = (VideoState*)arg;
//     AVFrame *video_frame = NULL;
//     AVFrame *vp = NULL;

//     AVRational tb = vs->video_st->time_base;
//     AVRational frame_rate = av_guess_frame_rate(vs->ic, vs->video_st, NULL);

//     video_frame = av_frame_alloc();

//     for(;;){
//         if(vs->quit){
//             break;
//         }

//         if(packet_queue_get(&vs->videoq, &vs->video_pkt, 0) < 0){
//             av_log(vs->video_ctx, AV_LOG_DEBUG, "video delay 10 ms\n");
//             SDL_Delay(10);
//             continue;
//         }

//         ret = avcodec_send_packet(&vs->video_ctx, &vs->video_pkt);
//         av_packet_unref(&vs->video_pkt); //释放资源
//         if(ret < 0){
//             av_log(vs->video_ctx, AV_LOG_ERROR, "Failed to send pkt to video decoder\n");
//             goto __ERROR;
//         }
//         while (ret > 0){
//             ret = avcodec_receive_frame(vs->video_ctx, video_frame);
//             if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
//                 break;
//             }else if (ret < 0){
//                 av_log(vs->video_ctx, AV_LOG_ERROR, "Failed to reveive frame from video decoder\n");
//                 ret = -1;
//                 goto __ERROR;
//             }
            
//             //video_display(vs);

//             //av sync
//             duration = (frame_rate.num && frame_rate.den ? av_d2q((AVRational){frame_rate.den, frame_rate.num}));
//             pts = (video_frame->pts == AV_NOPTS_VALUE) ? NAN:video_frame->pts*av_q2d(tb);
//             pts = synchronize_video(vs, video_frame, pts);

//             queue_picture(vs, video_frame, pts, duration, video_frame->pkt_pos);

//             av_frame_unref(video_frame);
//         }
//     }
//     ret = 0;
// __ERROR:

// }

// int audio_decode_frame(VideoState *vs){
//     int ret =-1;
//     int data_size = 0;
//     int len1, len2;

//     for (;;){
//         //从队列读取数据
//         if(packet_queue_get(&vs->audioq, &vs->audio_pkt, 0) <= 0){
//             av_log(NULL,AV_LOG_ERROR, "Could not get packet from audio queue");
//             break;
//         }

//         ret = avcodec_send_packet(vs->audio_ctx, &vs->audio_pkt);
//         av_packet_unref(&vs->audio_pkt);
//         if(ret < 0){
//             av_log(vs->audio_ctx, AV_LOG_ERROR, "Failed to send pkt to decoder !\n");
//             goto __OUT;
//         }

//         while (ret >= 0){
//             ret = avcodec_receive_frame(vs->audio_ctx, &vs->audio_frame);
//             if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
//                 break;
//             }else if(ret < 0){
//                 av_log(vs->audio_ctx, AV_LOG_ERROR, "Failed to receive frame from decoder !\n");
//                 goto __OUT;
//             }

//             if (!vs->audio_swr_ctx) {
//                 AVChannelLayout in_ch_layout, out_ch_layout;
//                 av_channel_layout_copy(&in_ch_layout, &vs->audio_ctx->ch_layout);
//                 av_channel_layout_copy(&out_ch_layout, &in_ch_layout);

//                 if (vs->audio_ctx->sample_fmt != AV_SAMPLE_FMT_S16){
//                     swr_alloc_set_opts2(&vs->audio_swr_ctx, &out_ch_layout, AV_SAMPLE_FMT_S16, vs->audio_ctx->sample_rate,
//                         &in_ch_layout, vs->audio_ctx->sample_fmt, vs->audio_ctx->sample_rate, 0, NULL);

//                     swr_init(vs->audio_swr_ctx);
//                 }
//             }

//             if (vs->audio_swr_ctx){
//                 const uint8_t **in = (const uint8_t**)vs->audio_frame->extended_data;
//                 uint8_t **out = &vs->audio_buf;
//                 int out_count = vs->audio_frame->nb_samples + 256;

//                 int out_size = av_samples_get_buffer_size(NULL, vs->audio_frame->ch_layout.nb_channels,
//                         vs->audio_frame->nb_samples, AV_SAMPLE_FMT_S16,0);
//                 av_fast_malloc(&vs->audio_buf, &vs->audio_buf_size, out_size);

//                 len2 = swr_convert(vs->audio_swr_ctx, out, out_count, in, vs->audio_frame->nb_samples);

//                 //输出
//                 data_size = len2 * vs->audio_frame->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
//             }
//             if(!isnan(vs->audio_frame->pts))
//                 vs->audio_clock = vs->audio_frame->pts + (double) vs->audio_frame->nb_samples / vs->audio_frame ;
//             else
//                 vs->audio_clock = NAN;

//             //release
//             av_frame_unref(&vs->audio_frame);

//             return data_size;
//         }
        
//     }
    

// __OUT:

// }

// void sdl_audio_callback(void *userdata, Uint8 *stream, int len){
//     VideoState *vs =(VideoState*)userdata;
//     int len1 = 0;
//     int audio_size = 0;

//     while (len > 0){
//         if (vs->audio_buf_index >= vs->audio_buf_size){
//             audio_size = audio_decode_frame(vs);
//             if(audio_size < 0){
//                 vs->audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
//                 vs->audio_buf = NULL;
//             }else{
//                 vs->audio_buf_size = audio_size;
//             }
//             vs->audio_buf_index = 0;
//         }

//         len1 = vs->audio_buf_size - vs->audio_buf_index;
//         if (len1 > len){
//             len1 = len;
//         }
//         if (vs->audio_buf){
//             memcpy(stream, (uint8_t*)vs->audio_buf + vs->audio_buf_index, len1);
//         }
        
//     }
    
// }

// static int audio_open(void* opaque, AVChannelLayout *wanted_channel_layout, int wanted_sample_rate){
//     SDL_AudioSpec wanted_spec, spec;
//     int wanted_nb_channels = wanted_channel_layout->nb_channels;

//     //set audio settings from codec info
//     wanted_spec.freq = wanted_sample_rate;
//     wanted_spec.format = AUDIO_S16SYS;
//     wanted_spec.channels = wanted_nb_channels;
//     wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//     wanted_spec.callback = sdl_audio_callback;
//     wanted_spec.userdata = (void*)opaque;

//     av_log(NULL, AV_LOG_INFO, "wanted spec: channels:%d, sample_fmt:%d, sample_rate:%d \n",
//         wanted_nb_channels, AUDIO_S16SYS, wanted_sample_rate);

//     if(SDL_OpenAudio(&wanted_spec, &spec) < 0){
//         av_log(NULL, AV_LOG_ERROR, "SDL_OpenAudio :%s\n", SDL_GetError());
//         return -1;
//     }
//     return spec.size;
// }

// int stream_component_open(VideoState *vs, int stream_index){
//     int ret = -1;

//     AVFormatContext *ic = vs->ic;
//     AVCodecContext *avctx = NULL;
//     const AVCodec *codec = NULL;

//     int sample_rate;
//     AVChannelLayout ch_layout = {0,};

//     AVStream *st = NULL;
//     int codec_id;

//     if(stream_index < 0 || stream_index >= ic->nb_streams){
//         return -1;
//     }

//     st = ic->streams[stream_index];
//     codec_id = st->codecpar->codec_id;
//     //1.find decoder
//     codec = avcodec_find_decoder(codec_id);
//     //2. alloc condec context
//     avctx = avcodec_alloc_context3(codec);
//     if(!avctx){
//         av_log(NULL, AV_LOG_ERROR, "Failed to alloc avctx, NO MEMORY\n");
//         goto __ERROR;
//     }
//     //3 copy parameter to codec context
//     if((ret = avcodec_parameters_to_context(avctx, st->codecpar)) < 0){
//         av_log(avctx, AV_LOG_ERROR, "Could not copy codec parameters to codec context!\n");
//         goto __ERROR;
//     }
//     //4 bind codec and codec context
//     if ((ret = avcodec_open2(avctx, codec, NULL)) < 0){
//         av_log(NULL, AV_LOG_ERROR, "Failed to bind codec ctx and codec !\n");
//         goto __ERROR;
//     }

//     switch (avctx->codec_type)
//     {
//     case AVMEDIA_TYPE_AUDIO:
//         sample_rate = avctx->sample_rate;
//         ret = av_channel_layout_copy(&ch_layout, &avctx->ch_layout);
//         if(ret < 0){
//             goto __ERROR;
//         }

//         //open audio
//         if((ret = audio_open(vs, &ch_layout, sample_rate)) < 0){
//             av_log(avctx, AV_LOG_ERROR, "Failed to open audio device!\n");
//             goto __ERROR;
//         }

//         vs->audio_buf_size = 0;
//         vs->audio_buf_index = 0;
//         vs->audio_st = st;
//         vs->audio_index = stream_index;
//         vs->audio_ctx = avctx;

//         //start play
//         SDL_PauseAudio(0);
//         break;
//     case AVMEDIA_TYPE_VIDEO:
//         vs->video_index = stream_index;
//         vs->video_st = st;
//         vs->video_ctx = avctx;

//         vs->frame_timer = (double)av_gettime()/1000000.0;
//         vs->frame_last_delay = 40e-3;//上一次渲染的delay时间
//         vs->video_current_pts_time = av_gettime(); //pts时间

//         //create decode thread
//         vs->decode_tid = SDL_CreateThread(decode_thread, "decode_thread", vs);
//         break;

//     default:
//     av_log(avctx, AV_LOG_ERROR, "Unknow Codec Type:%d\n", avctx->codec_type);
//         break;
//     }
//     ret = 0;
//     goto __END;

// __ERROR:
//     if(avctx) avcodec_free_context(&avctx);
// __END:
//     return ret;
// }

// int read_thread(void *args){
//     Uint32 pixformat;
//     int ret = -1;

//     int video_index = -1;
//     int audio_index = -1;

//     VideoState *vs = (VideoState*)args;
//     AVFormatContext *ic = NULL;
//     AVPacket *pkt = NULL;

//     pkt = av_packet_alloc();
//     if(!pkt){
//         av_log(NULL, AV_LOG_FATAL, "NO MEMORY!\n");
//         goto __ERROR;
//     }

//     //1.open media file
//     if((ret = avformat_open_input(&ic, vs->filename, NULL, NULL)) < 0 ){
//         av_log(NULL, AV_LOG_ERROR, "Could not open file:%s, %d(%s)\n", vs->filename,ret, av_err2str(ret));
//         goto __ERROR;
//     }
//     vs->ic = ic;

//     //2. extract media info
//     if(avformat_find_stream_info(ic, NULL) < 0){
//         av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
//         goto __ERROR;
//     }

//     //3. Find the first audio and video stream
//     for (size_t i = 0; i < ic->nb_streams; i++){
//         AVStream *st = ic->streams[i];
//         enum AVMediaType type = st->codecpar->codec_type;
//         if(type == AVMEDIA_TYPE_VIDEO && video_index < 0){
//             video_index = i;
//         }
//         if(type == AVMEDIA_TYPE_AUDIO && audio_index < 0){
//             audio_index = i;
//         }

//         //find video and audio
//         if(video_index > -1 && audio_index > -1){
//             break;
//         }
//     }

//     if(video_index < 0 || audio_index < 0){
//         av_log(NULL, AV_LOG_ERROR, "the file must be contains audio and video stream!\n");
//         goto __ERROR;
//     }

//     if(audio_index >= 0){ //open audio part
//         stream_component_open(vs, audio_index);
//     }
//     if(video_index >= 0){ //open video part
//         AVStream *st = ic->streams[video_index]; 
//         AVCodecParameters *codecpar = st->codecpar;
//         AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
//         if(codecpar->width)set_default_window_size(codecpar->width, codecpar->height, sar);
//         stream_component_open(vs, video_index);
//     }

//     //main decode loop
//     for (;;){
//         if (vs->quit) {
//             ret = -1;
//             goto __ERROR;
//         }

//         if(vs->audioq.size > MAX_QUEUE_SIZE || vs->videoq.size > MAX_QUEUE_SIZE){
//             SDL_Delay(10);//wait
//             continue;
//         }

//         //read packet
//         ret = av_read_frame(vs->ic, pkt);
//         if(ret < 0){
//             if(vs->ic->pb->error == 0){
//                 SDL_Delay(100); // wait for user input
//                 continue;
//             }else{
//                 break;
//             }
//         }

//         //save packet to queue
//         if(pkt->stream_index == vs->video_index){
//             packet_queue_put(&vs->videoq, pkt);
//         }else if (pkt->stream_index == vs->audio_index){
//             packet_queue_put(&vs->audioq, pkt);
//         }else{//discard other packets
//             av_packet_unref(pkt);
//         }
//     }

//     //all done: wait for it
//     while (!vs->quit){
//         SDL_Delay(100);
//     }
    
//     ret = 0;

// __ERROR:
//     if(pkt)av_packet_free(pkt);

//     if (ret != 0){
//         SDL_Event event;
//         event.type = FF_QUIT_EVENT;
//         event.user.data1 = vs;
//         SDL_PushEvent(&event);
//     }
    
//     return ret;

// }

// static Uint32 sdl_refresh_timer_callback(Uint32 interval, void* opaque){
//     SDL_Event event;
//     event.type = FF_REFRESH_EVENT;
//     event.user.data1 = opaque;
//     SDL_PushEvent(&event);
//     return 0; //0 means stop timer
// }

// static void schedule_refresh(VideoState *vs, int delay){
//     SDL_AddTimer(delay,  , vs);
// }

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

//     //初始化packet_queue
//     if(packet_queue_init(&vs->videoq) < 0 || packet_queue_init(&vs->audioq) < 0){
//         goto __ERROR;
//     }

//     //初始化video frame queue
//     if(frame_queue_init(&vs->pictq, VIDEO_PICTURE_QUEUE_SIZE) < 0){
//         goto __ERROR;
//     }

//     //set sync type
//     vs->av_sync_type = av_sync_type;

//     //create a new thread for reading auido and video data
//     vs->read_tid = SDL_CreateThread(read_thread, "read_thread", vs);
//     if(!vs->read_tid){
//         av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
//         goto __ERROR;
//     }

//     //set timer for show picture
//     schedule_refresh(vs, 400);

//     return vs;
// __ERROR:
//     stream_close(vs);
//     return NULL;
// }

// void video_refresh_timer(void *userdata){
//     VideoState *vs = (VideoState*)userdata;
//     Frame *vp = NULL;
//     double actual_delay,delay, sync_threshold, ref_clock, diff;
//     if (vs->video_st){
//         schedule_refresh(vs, 1);
//     }else{
        
//     }
    
// }

// static void sdl_event_loop(VideoState *vs){
//     SDL_Event event;
//     for(;;){
//         SDL_WaitEvent(&event);
//         switch (event.type)
//         {
//         case FF_QUIT_EVENT:
//         case SDL_QUIT:
//             vs->quit = 1;
//             do_exit(vs);
//             break;
//         case FF_REFRESH_EVENT:
//             video_refresh_timer(event.user.data1);   
//             break;
        
//         default:
//             break;
//         }
//     }
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