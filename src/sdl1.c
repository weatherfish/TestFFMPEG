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

    SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
    SDL_RenderClear(render);
    SDL_RenderPresent(render);

    // 添加事件循环
    int quit = 0;
    do
    {
        
    } while (quit);
    

    SDL_Event e;
  
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
    }

    SDL_DestroyRenderer(render);

__DWINDOW:
    SDL_DestroyWindow(window);

__EXIT:
    SDL_Quit();
    return 0;
}
