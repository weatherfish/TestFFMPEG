// // resample.cpp
// #include "resample.h"

// ReSample::ReSample(const std::string& deviceName, const std::string& outputFilePath)
//     : deviceName(deviceName), outputFilePath(outputFilePath) {
//     avdevice_register_all();
//     if (!openDevice() || !initVideoEncoder()) {
//         cleanup();
//         return;
//     }

//     outFile = fopen(outputFilePath.c_str(), "wb");
//     if (!outFile) {
//         std::cerr << "#### Failed to open output file" << std::endl;
//         cleanup();
//         return;
//     }

//     frame = allocFrame(frameWidth, frameHeight);
//     if (!frame) {
//         std::cerr << "#### Failed to allocate frame" << std::endl;
//         cleanup();
//         return;
//     }
// }

// ReSample::~ReSample() {
//     cleanup();
// }

// void ReSample::startRecording() {
//     readAndEncodeData();
// }

// bool ReSample::openDevice() {
//     AVDictionary* options = nullptr;
//     av_dict_set(&options, "video_size", "640x480", 0);
//     av_dict_set(&options, "framerate", "30", 0);
//     av_dict_set(&options, "pixel_format", "nv12", 0);

//     const AVInputFormat* inputFormat = av_find_input_format("AVFoundation");
//     int ret = avformat_open_input(&fmtContext, deviceName.c_str(), inputFormat, &options);
//     av_dict_free(&options);

//     if (ret < 0) {
//         char errStr[AV_ERROR_MAX_STRING_SIZE] = {0};
//         av_strerror(ret, errStr, AV_ERROR_MAX_STRING_SIZE);
//         std::cerr << "#### Failed to open device: " << errStr << std::endl;
//         return false;
//     }

//     return true;
// }

// bool ReSample::initVideoEncoder() {
//     const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
//     if (!codec) {
//         std::cerr << "#### Codec libx264 not found" << std::endl;
//         return false;
//     }

//     videoCodecContext = avcodec_alloc_context3(codec);
//     if (!videoCodecContext) {
//         std::cerr << "#### Could not allocate video codec context" << std::endl;
//         return false;
//     }

//     videoCodecContext->profile = FF_PROFILE_H264_HIGH_444;
//     videoCodecContext->level = 50;
//     videoCodecContext->width = frameWidth;
//     videoCodecContext->height = frameHeight;
//     videoCodecContext->time_base = {1, 30};
//     videoCodecContext->framerate = {30, 1};
//     videoCodecContext->gop_size = 250;
//     videoCodecContext->keyint_min = 25;
//     videoCodecContext->max_b_frames = 3;
//     videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
//     videoCodecContext->bit_rate = 600 * 1000; //码率

//     if (avcodec_open2(videoCodecContext, codec, nullptr) < 0) {
//         std::cerr << "#### Could not open video codec" << std::endl;
//         return false;
//     }

//     return true;
// }

// AVFrame* ReSample::allocFrame(int width, int height) {
//     AVFrame* frame = av_frame_alloc();
//     if (!frame) {
//         std::cerr << "#### Error allocating frame" << std::endl;
//         return nullptr;
//     }

//     frame->width = width;
//     frame->height = height;
//     frame->format = videoCodecContext->pix_fmt;

//     int ret = av_frame_get_buffer(frame, 32);
//     if (ret < 0) {
//         std::cerr << "#### Error allocating frame buffer" << std::endl;
//         av_frame_free(&frame);
//         return nullptr;
//     }

//     return frame;
// }

// void ReSample::readAndEncodeData() {
//     AVPacket packet;
//     int ret;

//     while (true) {
//         ret = av_read_frame(fmtContext, &packet);
//         if (ret < 0) {
//             if (ret == AVERROR_EOF) {
//                 // 达到输入数据流的结尾,正常退出循环
//                 av_log(nullptr, AV_LOG_INFO, "#### Read Done! \n");
//                 break;
//             }else if (ret == AVERROR(EAGAIN)) {
//                 // Resource temporarily unavailable, log and possibly retry
//                 av_log(nullptr, AV_LOG_ERROR, "#### Resource temporarily unavailable, will retry\n");
//                 continue; // Or implement a retry mechanism
//             } else {
//                 // 其他错误情况
//                 char errStr[AV_ERROR_MAX_STRING_SIZE] = {0};
//                 av_strerror(ret, errStr, AV_ERROR_MAX_STRING_SIZE);
//                 std::cerr << "#### Error reading frame: " << errStr << std::endl;
                
//                 // 可以在这里添加重试或其他错误处理逻辑
//                 // ...
                
//                 // 如果无法恢复,则退出循环
//                 break;
//             }
//         }

//         av_log(nullptr, AV_LOG_INFO, "#### Package Size: %d\n", packet.size);

//         av_frame_unref(frame);
//         ret = av_frame_make_writable(frame);
//         if (ret < 0) {
//             std::cerr << "#### Error making frame writable" << std::endl;
//             av_packet_unref(&packet);
//             continue;
//         }

//         // Copy video data to frame
//         memcpy(frame->data[0], packet.data, packet.size);

//         encodeData(frame);
//         av_packet_unref(&packet);
//     }

//     // Flush encoder
//     encodeData(nullptr);
// }

// void ReSample::encodeData(AVFrame* frame) {
//     fwrite(frame->data, 1, frame->pkt_size, outFile);
//     fflush(outFile);
//     return;
//     int ret = avcodec_send_frame(videoCodecContext, frame);
//     if (ret < 0) {
//         std::cerr << "#### Error sending frame to encoder" << std::endl;
//         return;
//     }

//     AVPacket packet;
//     av_init_packet(&packet);

//     while (ret >= 0) {
//         ret = avcodec_receive_packet(videoCodecContext, &packet);
//         if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//             break;
//         } else if (ret < 0) {
//             std::cerr << "#### Error encoding frame" << std::endl;
//             return;
//         }

//         fwrite(packet.data, 1, packet.size, outFile);
//         fflush(outFile);
//         av_packet_unref(&packet);
//     }
// }

// void ReSample::cleanup() {
//     if (outFile) {
//         fclose(outFile);
//     }

//     if (frame) {
//         av_frame_free(&frame);
//     }

//     if (videoCodecContext) {
//         avcodec_free_context(&videoCodecContext);
//     }

//     if (fmtContext) {
//         avformat_close_input(&fmtContext);
//     }
// }