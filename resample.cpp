#include "resample.h"

resample::resample(/* args */)
{
}

resample::~resample()
{
}

void resample::setStatus(int status){
    
}

void resample::recAudio(void){

    int ret = 0;

    char errors[1024] = {0,};

    AVFormatContext *fmtContext;
    AVDictionary *options;

    AVPacket pkt;

    char *deviceName = ":0";

    av_log_set_level(AV_LOG_DEBUG);

    resStatus = 1;

    avdevice_register_all();

    const AVInputFormat *iformat = av_find_input_format("avfoundation");

    if((ret = avformat_open_input(&fmtContext, deviceName, iformat, &options)) < 0){
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return;
    }

    char *out = "audio.pcm";
    FILE *outFile = fopen(out, "wb+");

    SwrContext *swrContext;
    AVChannelLayout channelLayout = AV_CHANNEL_LAYOUT_STEREO;
    
    swr_alloc_set_opts2(&swrContext,  &channelLayout, AV_SAMPLE_FMT_S16, 441000, 
                         &channelLayout, AV_SAMPLE_FMT_FLT, 441000, 0, NULL);

    if(!swrContext){

    }

    if(swr_init(swrContext) < 0 ){

    }

    uint8_t **src_data = NULL;
    int src_linesize = 0;
    //输入缓存区
    av_samples_alloc_array_and_samples(&src_data, &src_linesize, 2, 512, AV_SAMPLE_FMT_FLT, 0);

    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    //输出缓存区
    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, 2, 512, AV_SAMPLE_FMT_S16, 0);

    while ((ret = av_read_frame(fmtContext, &pkt)) == 0 && resStatus)
    {
        av_log(NULL, AV_LOG_INFO, "Packaeges Size is %d(%p)]\n", pkt.size, pkt.data);

        memcpy(src_data[0], pkt.data, pkt.size);
        //重采样
        swr_convert(swrContext, dst_data, 512, (uint8_t * const *)src_data, 512);

        // fwrite(pkt.data, 1, pkt.size, outFile);
        fwrite(dst_data[0], 1, dst_linesize, outFile);
        fflush(outFile);
        av_packet_unref(&pkt);
    }

    fclose(outFile);

    if(src_data){
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);

    if(dst_data){
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);

    swr_free(&swrContext);

    avformat_close_input(&fmtContext);
    av_log(NULL, AV_LOG_DEBUG, "finish \n");
    
}