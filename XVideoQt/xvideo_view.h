#ifndef XVIDEO_VIEW_H
#define XVIDEO_VIEW_H
#include <mutex>
#include <chrono>
#include <fstream>

struct AVFrame;

class XVideoView
{
public:
    XVideoView();

    enum VideoFormat{  //和 AVPixelFormat一致
        YUV420P = 0,
        ARGB = 25,
        RGBA = 26,
        BGRA = 28,
    };

    enum RenderType{
        SDL = 0,
    };

    static XVideoView* create(RenderType type = SDL);

    //初始化渲染窗口
    virtual bool Init(int w, int h, VideoFormat format = VideoFormat::RGBA) = 0;

    //绘制接口 线程安全
    virtual bool Draw(const unsigned char* data, int linesize = 0) = 0;

    virtual bool Draw(const unsigned char* y, int y_pitch, const unsigned char* u, int u_pitch, const unsigned char* v, int v_pitch) = 0;

    virtual bool DrawFrame(const AVFrame *frame);

    virtual void Close() = 0;

    //处理窗口退出
    virtual bool IsExit() = 0;

    void scale(int width, int height){
        // render_width_ = width;
        // render_height_ = height;
    }

    int renderFps(){return renderFps_;}

    bool Open(std::string filepath);

    AVFrame* Read(); //读取一帧数据并维护AVFrame空间，每次调用会覆盖上一次的AVFrame

    void setWinID(void* winID){this->winID_ = winID;}

    void* getWinId(){return winID_;}

protected:
    int renderFps_ = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> beginMs; //计时开始时间
    int count_  = 0; //统计显示次数

    int width_ = 0; //材质大小
    int height_ = 0;
    VideoFormat format_ = RGBA;
    void* winID_ = nullptr; // 窗口句柄

    // int render_width_ = 0; //显示的宽高
    // int render_height_ = 0;
    std::mutex mtx_;

private:
    std::ifstream ifs_;
    AVFrame* frame_ = nullptr;


};

#endif // XVIDEO_VIEW_H
