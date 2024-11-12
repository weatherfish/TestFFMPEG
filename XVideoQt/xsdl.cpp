#include "xsdl.h"
#include <SDL2/SDL.h>
#include <iostream>

XSDL::XSDL() {}

static bool InitVideo(){
    static bool isFirst = true;
    static std::mutex mtx;
    std::unique_lock<std::mutex> sdl_lock_(mtx);
    if(!isFirst)return true;
    isFirst = false;
    if(SDL_Init(SDL_INIT_VIDEO)){
        std::cout<<SDL_GetError()<<std::endl;
        return false;
    }
    //设置抗锯齿算法
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    return true;
}

bool XSDL::Init(int w, int h, VideoFormat format){
    if(w <= 0 || h <=0)return false;
    InitVideo();
    std::unique_lock<std::mutex> sdl_lock_(mtx_); //确保现线程安全
    width_ = w;
    height_ = h;
    format_ = format;
    scale(width_, height_);  //设置为窗口的宽高

    std::cout<<"width="<<w<<", height="<<h<<",fmt="<<format<<std::endl;

    if(!win_){
        if(!winID_){
            win_ = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
        }else{
            win_ = SDL_CreateWindowFrom(winID_);
        }
    }
    if(!win_){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    // SDL_SetWindowOpacity(win_, 0.0); // 设置透明度为 0.0（完全透明）

    if(texture_){
        SDL_DestroyTexture(texture_);
    }
    if(render_){
        SDL_DestroyRenderer(render_);
    }

    render_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
    if(!render_){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    // SDL_SetRenderDrawBlendMode(render_, SDL_BLENDMODE_NONE);

    unsigned int sdlFmt =  SDL_PIXELFORMAT_RGBA8888;
    switch (format_) {
    case RGBA:
        sdlFmt = SDL_PIXELFORMAT_RGBA32;
        break;
    case ARGB:
        sdlFmt = SDL_PIXELFORMAT_ARGB32;
        break;
    case BGRA:
        sdlFmt = SDL_PIXELFORMAT_BGRA32;
        break;
    case YUV420P:
        sdlFmt = SDL_PIXELFORMAT_IYUV;
        break;
    default:
        break;
    }
    texture_ = SDL_CreateTexture(render_, sdlFmt, SDL_TEXTUREACCESS_STREAMING, w, h);
    if(!texture_){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    return true;
}

bool XSDL::Draw(const unsigned char* data, int linesize){
    if(!data)return false;
    std::unique_lock<std::mutex> sdl_lock_(mtx_);
    if(!texture_ || !render_ || !win_ || width_ <= 0 || height_ <= 0) return false;

    if(linesize <= 0){
        switch (format_) {
        case RGBA:
        case ARGB:
        case BGRA:
            linesize = width_ * 4;
            break;
        case YUV420P:
            linesize = width_;
            break;
        default:
            break;
        }
    }
    if(linesize <= 0) return false;

    auto re = SDL_UpdateTexture(texture_, NULL, data, linesize);
    if(re != 0){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    SDL_RenderClear(render_);

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width_;
    rect.h = height_;

    re = SDL_RenderCopy(render_, texture_, nullptr, &rect);
    if(re != 0){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    SDL_RenderPresent(render_);

    return true;
}

bool XSDL::Draw(const unsigned char* y, int y_pitch, const unsigned char* u, int u_pitch, const unsigned char* v, int v_pitch){
    if(!y || !u || !v)return false;

    std::unique_lock<std::mutex> sdl_lock_(mtx_);
    if(!texture_ || !render_ || !win_ || width_ <= 0 || height_ <= 0) return false;

    auto re = SDL_UpdateYUVTexture(texture_, NULL, y, y_pitch, u, u_pitch, v, v_pitch);
    if(re != 0){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    // SDL_SetRenderDrawColor(render_, 0, 0, 0, 0); // 设置为白色

    SDL_RenderClear(render_);

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width_;
    rect.h = height_;

    re = SDL_RenderCopy(render_, texture_, nullptr, &rect);
    if(re != 0){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    SDL_RenderPresent(render_);
    return true;
}

// bool XSDL::Draw(const AVFrame *frame){

//     return true;
// }

void XSDL::Close(){
    std::unique_lock<std::mutex> sdl_lock_(mtx_); //确保现线程安全
    if(texture_){
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if(render_){
        SDL_DestroyRenderer(render_);
        render_ = nullptr;
    }
    if(win_){
        SDL_DestroyWindow(win_);
        win_ = nullptr;
    }
}

bool XSDL::IsExit(){
    SDL_Event ev;
    SDL_WaitEventTimeout(&ev, 1);
    if(ev.type == SDL_QUIT)return true;
    return false;
}
