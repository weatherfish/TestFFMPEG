#ifndef GET_INFO_H
#define GET_INFO_H

extern "C"{
    #include <libavutil/avutil.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}
typedef struct __AVGeneralMediaInfo{
    char filepath[1024];   //文件路径
    int64_t duration;   //时长 微秒  1000000
    int64_t totalBitrate;   //总码率
    int videoStreamIndex;   //视频流索引
    int audioStreamIndex;   //音频流索引

    char videoCodecName[256];   //视频编码器名
    int width;
    int height;
    double frameRate;  //帧率

    char audioCodecName[256];   //音频编码器名
    int sampleRate; //采样率
    int channels;   //声道数
} AVGeneralMediaInfo;

void get_decoder_name(AVFormatContext *formatContext, AVGeneralMediaInfo *info, int type){
    int index = -1;
    if(type == 0){//视频
        index = info->videoStreamIndex;
    }else if(type == 1){//音频
        index = info->audioStreamIndex;
    }
    if(index >=0 ){
        const AVCodec *codec = avcodec_find_decoder(formatContext->streams[index]->codecpar->codec_id);
         if(type == 0){//视频
            strcpy(info->videoCodecName, codec->name);
            printf(" videoCodecName = %s\n", info->videoCodecName);
        }else if(type == 1){//音频
            strcpy(info->audioCodecName, codec->name);
            printf(" audioCodecName = %s\n", info->audioCodecName);
        }
    }
}

void get_avgeneral_mediainfo(AVGeneralMediaInfo *info, const char* filepath){
    int ret = -1;
    if(info == NULL || filepath == NULL){
        return;
    }
    AVFormatContext *formatContext = NULL;

    ret = avformat_open_input(&formatContext, filepath, NULL, NULL);
    if( ret<0 ){
        printf("Error open %s\n", filepath);
        return;
    }

    ret = avformat_find_stream_info(formatContext, NULL);
    if( ret<0 ){
        printf("Error find stream info %s\n", filepath);
        return;
    }

    av_dump_format(formatContext, 0, filepath, 0);

    info->duration = formatContext->duration;
    info->totalBitrate = formatContext->bit_rate;

    for (size_t i = 0; i < formatContext->nb_streams; i++)
    {
        AVStream *avs = formatContext->streams[i];
        if(avs->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            info->videoStreamIndex = i;
            info->width = avs->codecpar->width;
            info->height = avs->codecpar->height;

            if(avs->avg_frame_rate.num != 0 &&avs->avg_frame_rate.den != 0){
                info->frameRate = (double)avs->avg_frame_rate.num/(double)avs->avg_frame_rate.den;
            }

            printf(" width = %d, height = %d, frameRate= %lf\n" , info->width, info->height, info->frameRate);
        }else if(avs->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            info->audioStreamIndex = i;
            info->channels = avs->codecpar->ch_layout.nb_channels;
            info->sampleRate = avs->codecpar->sample_rate;
            printf(" channels = %d, sampleRate = %d\n", info->channels, info->sampleRate);
        }
    }

    // if(info->videoStreamIndex >=0 ){
    //     const AVCodec *codec = avcodec_find_decoder(formatContext->streams[info->videoStreamIndex]->codecpar->codec_id);
    //     strcpy(info->videoCodecName, codec->name);
    //     printf(" videoCodecName = %s\n", info->videoCodecName);
    // }

    get_decoder_name(formatContext, info, 0);
    get_decoder_name(formatContext, info, 1);
    


    avformat_close_input(&formatContext);

}

#endif