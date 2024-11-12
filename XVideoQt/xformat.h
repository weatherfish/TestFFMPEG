#ifndef XFORMAT_H
#define XFORMAT_H

#include <mutex>


struct AVFormatContext;
struct AVCodecParameters;
struct AVPacket;

struct XRational{
    int num;
    int den;
};

class XFormat
{
public:

    bool CopyParams(int stream_index, AVCodecParameters* dst);

    //设置null相当于关闭
    void setC(AVFormatContext* c);

    int audio_index(){
        return audio_index_;
    }
    int video_index(){
        return video_index_;
    }
    XRational video_time_base(){
        return video_time_base_;
    }
    XRational audio_time_base(){
        return audio_time_base_;
    }

    bool RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base);

    void setTimeOutMs(int ms);

protected:
    int video_index_ = 0; //在strean中的索引
    int audio_index_ = 1;

    std::mutex mtx_;
    AVFormatContext* c_;

    XRational video_time_base_ = {1, 25};
    XRational audio_time_base_ = {1, 9600};

    int video_codec_id_; //编码器索引
    int time_out_ms_;//超时时间
    long long last_time_; //上次接收到数据的时间
};

#endif // XFORMAT_H
