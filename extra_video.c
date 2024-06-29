#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int dosth(int argc, char *argv[]){
    //1、处理参数
    char* src;
    char* dst;

    int ret = -1;
    int videoIndex = -1;
    AVFormatContext *formatContext = NULL;
    AVFormatContext *dstFormatContext = NULL;
    AVOutputFormat *outFormat = NULL;
    AVStream *outStream = NULL;
    AVStream *inStream = NULL;
    AVPacket packet;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 3){
        av_log(NULL, AV_LOG_INFO, "args must more then two");
        exit(-1);
    }

    src = argv[1];
    dst = argv[2];

    //2.打开多媒体文件
    ret = avformat_open_input(formatContext, src, NULL, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }

    //3、找到视频流
    videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoIndex < 0){
        av_log(formatContext, AV_LOG_ERROR, "Does not include video stream\n");
        goto _ERROR;
    }

    //4、打开目的文件的上下文
    dstFormatContext = avformat_alloc_context();
      if(!dstFormatContext){
        av_log(NULL, AV_LOG_ERROR, "No memory! \n");
        goto _ERROR;
    }
    outFormat = av_guess_format(NULL, dst, NULL);
    formatContext->oformat = outFormat; //目标格式

    //5、创建新的视频流为目的文件
    outStream = avformat_new_stream(dstFormatContext, NULL);

    //6、设置输出视频参数
    inStream = formatContext->streams[videoIndex];
    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
    outStream->codecpar->codec_tag = 0;//根据多媒体文件自动适配编解码器
    //绑定上下文
    ret = avio_open2(&dstFormatContext->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if(ret < 0){
        av_log(dstFormatContext, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    //7、写多媒体文件头到目的文件
    ret = avformat_write_header(dstFormatContext, NULL);
    if(ret < 0){
        av_log(dstFormatContext, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    //8、从源读视频数据到目的文件
    while (av_read_frame(formatContext, &packet))
    {
        if(packet.stream_index == videoIndex){
            //pts换算
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = packet.dts;
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.stream_index = 0; //只有一路流
            packet.pos = -1;//相对位置，内部自行计算
            av_interleaved_write_frame(dstFormatContext, &packet);
            av_packet_unref(&packet);//释放引用
        }
    }
    
    //9、写多媒体文件尾
    av_write_trailer(dstFormatContext);

    //10、释放资源
_ERROR:
    if(formatContext){
        avformat_free_context(formatContext);
        formatContext = NULL;
    }
    if(dstFormatContext->pb){
        avio_close(dstFormatContext->pb);
    }
    if(dstFormatContext){
        avformat_free_context(dstFormatContext);
        dstFormatContext = NULL;
    }

    return 0;
}