#include <SDL.h>
#include <stdio.h>

int sdl1(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* render = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("TestSDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        goto __EXIT;
    }

    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!render) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        goto __DWINDOW;
    }

    // SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
    // SDL_RenderClear(render);
    // SDL_RenderPresent(render);

    // 添加事件循环
    int quit = 1;
    SDL_Event event;

    SDL_Texture *texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    if (!texture)
    {
        printf("Failed to create texture\n");
        goto __RENDER;
    }
    

    do{  
        if (SDL_PollEvent(&event) != 0) {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = 0;
                break;
            
            default:
                SDL_Log("event type is %d ", event.type);
                break;
            }
        }

        SDL_Rect rect;
        rect.w = 30;
        rect.h = 30;
        rect.x = rand() % 600;
        rect.y = rand() % 440;

        SDL_SetRenderTarget(render, texture);//改变渲染目标

        SDL_SetRenderDrawColor(render, 0, 0, 0, 0);

        SDL_RenderClear(render);

        SDL_RenderDrawRect(render, &rect);

        SDL_SetRenderDrawColor(render, 255, 0, 0, 0);

        SDL_RenderFillRect(render, &rect);

        SDL_SetRenderTarget(render, NULL);  //恢复默认渲染目标

        SDL_RenderCopy(render, texture, NULL, NULL);//拷贝纹理给显卡

        SDL_RenderPresent(render);//最终输出

    } while (quit);

    SDL_DestroyTexture(texture);

__RENDER:
    SDL_DestroyRenderer(render);

__DWINDOW:
    SDL_DestroyWindow(window);

__EXIT:
    SDL_Quit();
    return 0;
}
