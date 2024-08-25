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
    return true;
}

bool XSDL::Init(int w, int h, VideoFormat format , void* winId){
    if(w <= 0 || h <=0)return false;
    InitVideo();
    std::unique_lock<std::mutex> sdl_lock_(mtx_); //确保现线程安全
    width_ = w;
    height_ = h;
    std::cout<<"width="<<w<<", height="<<h<<std::endl;
    format_ = format;
    if(!win_){
        if(!winId){
            win_ = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
        }else{
            win_ = SDL_CreateWindowFrom(winId);
        }
    }
    if(!win_){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    render_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
    if(!render_){
        std::cerr<<SDL_GetError()<<std::endl;
        return false;
    }

    unsigned int sdlFmt =  SDL_PIXELFORMAT_RGBA8888;
    switch (format_) {
    case RGBA:
        sdlFmt = SDL_PIXELFORMAT_RGBA8888;
        break;
    case ARGB:
        sdlFmt = SDL_PIXELFORMAT_ARGB8888;
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
