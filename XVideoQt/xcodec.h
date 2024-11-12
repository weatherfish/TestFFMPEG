#ifndef XCODEC_H
#define XCODEC_H

#include <mutex>


struct AVCodecContext;
struct AVPacket;
struct AVFrame;

class XCodec
{
public:
    //创建编码器上下文
    static AVCodecContext* Create(int codeId, bool isEncode);
    void setContext(AVCodecContext* c);  //设置编码器上下文，如果原数据不空，需要清理，线程安全
    bool SetOpt(const char* key, const char* val); //设置编码参数
    bool SetOpt(const char* key, int val); //设置编码参数

    bool OpenEncoder();//打开编码器

    AVFrame* CreateFrame();

protected:
    AVCodecContext* c_; //编码器上下文和锁
    std::mutex mtx_;
};

#endif // XCODEC_H
