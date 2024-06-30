#include <stdio.h>
#include <stdlib.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int cut(int argc, char *argv[]) {
    // 1. 处理参数
    char *src;
    char *dst;
    double beginTime = 0;
    double endTime = 0;

    int ret = -1;
    AVFormatContext *formatContext = NULL;
    AVFormatContext *dstFormatContext = NULL;
    AVPacket packet;

    int *streamMap = NULL;
    int stream_index = 0;

    av_log_set_level(AV_LOG_DEBUG);

    //cut src dst start end
    if (argc < 5) {
        av_log(NULL, AV_LOG_INFO, "参数必须多于4个\n");
        exit(-1);
    }

    src = argv[1];
    dst = argv[2];
    beginTime = atof(argv[3]);
    endTime = atof(argv[4]);

    // 2. 打开多媒体文件
    ret = avformat_open_input(&formatContext, src, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }

    // 4. 打开目标文件上下文
    avformat_alloc_output_context2(&dstFormatContext, NULL, NULL, dst);
    if (!dstFormatContext) {
        av_log(NULL, AV_LOG_ERROR, "内存不足\n");
        goto _ERROR;
    }

    streamMap = av_calloc(formatContext->nb_streams, sizeof(int));
    if (!streamMap) {
        av_log(NULL, AV_LOG_ERROR, "内存不足\n");
        goto _ERROR;
    }
    for (int i = 0; i < formatContext->nb_streams; i++) {
        AVStream *inStream = formatContext->streams[i];
        AVCodecParameters *params = inStream->codecpar;
        if (params->codec_type != AVMEDIA_TYPE_AUDIO &&
            params->codec_type != AVMEDIA_TYPE_VIDEO &&
            params->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streamMap[i] = -1;
            continue;
        }
        streamMap[i] = stream_index++;

        // 5. 为目标文件创建新的视频流
        AVStream *outStream = avformat_new_stream(dstFormatContext, NULL);
        if (!outStream) {
            av_log(dstFormatContext, AV_LOG_ERROR, "内存不足\n");
            goto _ERROR;
        }

        avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        outStream->codecpar->codec_tag = 0; // 根据多媒体文件自动适配编解码器
    }

    // 绑定上下文
    ret = avio_open2(&dstFormatContext->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0) {
        av_log(dstFormatContext, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    // 7. 写多媒体文件头到目标文件
    ret = avformat_write_header(dstFormatContext, NULL);
    if (ret < 0) {
        av_log(dstFormatContext, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    //AVSEEK_FLAG_BACKWARD 向后找，从i帧开始读取
    ret = av_seek_frame(formatContext, -1, beginTime * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    int64_t *dtsBeginTime = av_calloc(formatContext->nb_streams, sizeof(int64_t));
    int64_t *ptsBeginTime = av_calloc(formatContext->nb_streams, sizeof(int64_t));
    for (int i = 0; i < formatContext->nb_streams; i++) {
        dtsBeginTime[i] = -1;
        ptsBeginTime[i] = -1;
    }

    // 8. 从源文件读取数据到目标文件
    while (av_read_frame(formatContext, &packet) >= 0) {
        AVStream *inStream, *outStream;
        if (dtsBeginTime[packet.stream_index] == -1 && packet.dts > 0) {
            dtsBeginTime[packet.stream_index] = packet.dts;
        }
        if (ptsBeginTime[packet.stream_index] == -1 && packet.pts > 0) {
            ptsBeginTime[packet.stream_index] = packet.pts;
        }

        inStream = formatContext->streams[packet.stream_index];
        
        if (av_q2d(inStream->time_base) * packet.pts > endTime) {
            av_log(dstFormatContext, AV_LOG_INFO, "Success");
            break;
        }

        if (streamMap[packet.stream_index] < 0) {
            av_packet_unref(&packet);
            continue;
        }

        packet.pts = packet.pts - ptsBeginTime[packet.stream_index];
        packet.dts = packet.dts - dtsBeginTime[packet.stream_index];

        packet.stream_index = streamMap[packet.stream_index];

        outStream = dstFormatContext->streams[packet.stream_index];
        av_packet_rescale_ts(&packet, inStream->time_base, outStream->time_base);

        // 确保 DTS 和 PTS 是递增的
        if (packet.dts < 0) {
            packet.dts = 0;
        }
        if (packet.pts < packet.dts) {
            packet.pts = packet.dts;
        }

        packet.pos = -1; // 相对位置，内部自行计算
        ret = av_interleaved_write_frame(dstFormatContext, &packet);
        if (ret < 0) {
            av_log(dstFormatContext, AV_LOG_ERROR, "Error muxing packet: %s\n", av_err2str(ret));
            break;
        }
        av_packet_unref(&packet); // 释放引用
    }

    // 9. 写多媒体文件尾
    av_write_trailer(dstFormatContext);

    // 10. 释放资源
_ERROR:
    if (formatContext) {
        avformat_free_context(formatContext);
        formatContext = NULL;
    }
    if (dstFormatContext && dstFormatContext->pb) {
        avio_close(dstFormatContext->pb);
    }
    if (dstFormatContext) {
        avformat_free_context(dstFormatContext);
        dstFormatContext = NULL;
    }
    if (streamMap) {
        av_free(streamMap);
        streamMap = NULL;
    }
    if (ptsBeginTime) {
        av_free(ptsBeginTime);
        ptsBeginTime = NULL;
    }
    if (dtsBeginTime) {
        av_free(dtsBeginTime);
        dtsBeginTime = NULL;
    }

    return 0;
}
