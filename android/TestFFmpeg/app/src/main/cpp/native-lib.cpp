#include <jni.h>
#include <string>

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#include "FFLog.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_felix_test_ffmpeg_utils_FFUtils_playVideo(JNIEnv *env, jobject thiz, jstring video_path, jobject surface) {
    const char *videoPath = env->GetStringUTFChars(video_path, nullptr);
    ALOGI("playVideo :%s", videoPath);

    if (videoPath == nullptr) {
        ALOGE("videoPath is null");
        return;
    }

    AVFormatContext *formatContext = avformat_alloc_context();

    //open video file
    ALOGI("Open video file");
    if (avformat_open_input(&formatContext, videoPath, nullptr, nullptr) != 0) {
        ALOGE("Cannot open video file %s\n", videoPath);
        return;
    }

    //retrieve stream information
    ALOGI("retrieve stream information");
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        ALOGE("Cannot find stream information\n");
        return;
    }

    //Find the first video stream
    ALOGI("Find video stream");
    int video_index = -1;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    if (video_index == -1) {
        ALOGE("no video stream found\n");
        return;
    }

    //Get a pointer to the codec context for the video stream
    ALOGI("Get a pointer to the codec context for the video stream");
    AVCodecParameters *codecParameters = formatContext->streams[video_index]->codecpar;

    ALOGI("Find the decoder for the video stream");
    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        ALOGE("Codec not found\n");
        return;
    }

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (codecContext == nullptr) {
        ALOGE("AVCodecContext not found\n");
        return;
    }

    //fill codecContext according to codecParameters
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        ALOGE("Fill AVCodecContext failed\n");
        return;
    }

    //init codec context
    ALOGI("Init Codec");
    if (avcodec_open2(codecContext, codec, nullptr)) {
        ALOGE("Init AVCodecContext failed\n");
        return;
    }

    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;

    //Allocate av packet
    AVPacket *packet = av_packet_alloc();
    if (packet == nullptr) {
        ALOGE("Could not allocate av packet\n");
        return;
    }

    ALOGI("Allocate video frame");
    AVFrame *frame = av_frame_alloc();

    ALOGI("Allocate render frame");
    AVFrame *renderFrame = av_frame_alloc();

    if (frame == nullptr || renderFrame == nullptr) {
        ALOGE("Could not allocate video frame\n");
        return;
    }

    ALOGI("Determine required buffer size and allocate buffer");
    int size = av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height, 1);
    auto *buffer = (uint8_t *) av_malloc(size * sizeof(uint8_t));
    av_image_fill_arrays(renderFrame->data, renderFrame->linesize, buffer, dstFormat,
                         codecContext->width, codecContext->height, 1);

    //init swsContext
    ALOGI("init swsContext");
    struct SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
                                                   codecContext->width, codecContext->height, dstFormat, SWS_BILINEAR,
                                                   nullptr, nullptr, nullptr);
    if (swsContext == nullptr) {
        ALOGE("init swsContext failed\n");
        return;
    }

    ALOGI("native window");
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer windowBuffer;

    ALOGI("get video width height");
    int videoWidth = codecContext->width;
    int videoHeight = codecContext->height;

    ALOGI("get video width = %d,  height = %d", videoWidth, videoHeight);

    ALOGI("set native window");
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);

    ALOGI("render frame");
    while (av_read_frame(formatContext, packet) == 0) {
        //IS this a packet from video stream?
        if (packet->stream_index == video_index) {
            //send origin data to decoder
            int sendPacketState = avcodec_send_packet(codecContext, packet);
            if (sendPacketState == 0) {
                ALOGD("向解码器发送数据");
                int receiveFrameState = avcodec_receive_frame(codecContext, frame);
                if (receiveFrameState == 0) {
                    ALOGD("从解码器接收数据");
                    ANativeWindow_lock(nativeWindow, &windowBuffer, nullptr);

                    //格式转换
                    sws_scale(swsContext, (uint8_t const *const *) frame->data,
                              frame->linesize, 0, codecContext->height,
                              renderFrame->data, renderFrame->linesize);

                    //获取stride
                    auto *dst = (uint8_t *) windowBuffer.bits;
                    auto *src = renderFrame->data[0];
                    int dstStride = windowBuffer.stride * 4;
                    int srcStride = renderFrame->linesize[0];

                    //由于window的stride 和帧的stride不同，因此需要逐行复制
                    for (int i = 0; i < videoHeight; ++i) {
                        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
                    }

                    ANativeWindow_unlockAndPost(nativeWindow);
                } else if (receiveFrameState == AVERROR(EAGAIN)) {
                    ALOGD("从解码器接收数据失败： AVERROR(EAGAIN) ");
                } else if (receiveFrameState == AVERROR_EOF) {
                    ALOGD("从解码器接收数据失败： AVERROR_EOF");
                } else if (receiveFrameState == AVERROR(EINVAL)) {
                    ALOGD("从解码器接收数据失败： AVERROR(EINVAL)");
                } else {
                    ALOGD("从解码器接收数据失败：UNKNOWN");
                }
            } else if (sendPacketState == AVERROR(EAGAIN)) { //发送数据被拒绝
                ALOGD("向解码器发送数据失败： AVERROR(EAGAIN) ");
            } else if (sendPacketState == AVERROR_EOF) {
                ALOGD("向解码器发送数据失败： AVERROR_EOF");
            } else if (sendPacketState == AVERROR(EINVAL)) {//编码器没有打开
                ALOGD("向解码器发送数据失败： AVERROR(EINVAL)");
            } else if (sendPacketState == AVERROR(ENOMEM)) {//数据包无法打开解码器队列
                ALOGD("向解码器发送数据失败： AVERROR(ENOMEM)");
            } else {
                ALOGD("向解码器发送数据失败：UNKNOWN");
            }
        }
        av_packet_unref(packet);
    }

    ALOGI("release memeory");
    ANativeWindow_release(nativeWindow);
}