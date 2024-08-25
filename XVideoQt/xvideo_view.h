#ifndef XVIDEO_VIEW_H
#define XVIDEO_VIEW_H
#include <mutex>

class XVideoView
{
public:
    XVideoView();
    enum VideoFormat{
        RGBA = 0,
        ARGB,
        YUV420P,
    };

    enum RenderType{
        SDL = 0,
    };

    static XVideoView* create(RenderType type = SDL);

    //初始化渲染窗口
    virtual bool Init(int w, int h, VideoFormat format = VideoFormat::RGBA, void* winId = nullptr) = 0;

    //绘制接口 线程安全
    virtual bool Draw(const unsigned char* data, int linesize = 0) = 0;

    virtual void Close() = 0;

    //处理窗口退出
    virtual bool IsExit() = 0;

    void scale(int width, int height){
        render_width_ = width;
        render_height_ = height;
    }


protected:
    int width_ = 0; //材质大小
    int height_ = 0;
    VideoFormat format_ = RGBA;

    int render_width_ = 0; //显示的宽高
    int render_height_ = 0;
    std::mutex mtx_;


};

#endif // XVIDEO_VIEW_H
