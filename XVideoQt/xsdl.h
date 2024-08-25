#ifndef XSDL_H
#define XSDL_H

#include "xvideo_view.h"
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class XSDL :public XVideoView
{
public:
    XSDL();
    //初始化渲染窗口
    bool Init(int w, int h, VideoFormat format = VideoFormat::RGBA, void* winId = nullptr) override;

    //绘制接口 线程安全
    bool Draw(const unsigned char* data, int linesize = 0) override;

private:
    SDL_Window *win_ = nullptr;
    SDL_Renderer *render_ = nullptr;
    SDL_Texture *texture_ = nullptr;
};

#endif // XSDL_H
