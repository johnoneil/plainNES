#include "gui.h"
#include "render.h"
#include <SDL.h>
#undef main
#include <cstring>

namespace GUI {

SDL_Window *mainwindow;
SDL_Window *PTwindow;
SDL_Renderer *mainrenderer;
SDL_Renderer *PTrenderer;
SDL_Texture *maintexture;
SDL_Texture *PTtexture;
SDL_Event event;
const uint8_t *kbState = SDL_GetKeyboardState(NULL);

uint32_t mainpixelMap[256 * 240];
uint32_t PTpixelMap[276 * 128];
bool quit = false;

int init()
{
    RENDER::init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    mainwindow = SDL_CreateWindow("plainNES",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                256,240,
                0);
    /*PTwindow = SDL_CreateWindow("plainNES - Pattern Table",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                276,128,
                0);*/
    
    mainrenderer = SDL_CreateRenderer(mainwindow, -1, 0);
    //PTrenderer = SDL_CreateRenderer(PTwindow, -1, 0);
    maintexture = SDL_CreateTexture(mainrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    //PTtexture = SDL_CreateTexture(PTrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 276, 128);		

    for(int i=0; i<256*240; ++i) {
        mainpixelMap[i] = 0xFF000000;
    }
    for(int i=0; i<276*128; ++i) {
        PTpixelMap[i] = 0xFF000000;
    }

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap, 256*4);
    //SDL_UpdateTexture(PTtexture, NULL, PTpixelMap, 276*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    //SDL_RenderCopy(PTrenderer, PTtexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);
    //SDL_RenderPresent(PTrenderer);

    SDL_Delay(5000);
    return 0;
}

void update(uint8_t *screenBuffer, uint8_t *controllerPtr)
{
    RENDER::convertNTSC2ARGB(mainpixelMap, screenBuffer);

    while( SDL_PollEvent(&event) != 0) {
        if(event.type == SDL_QUIT) {
            quit = true;
            return;
        }
        else if(event.type == SDL_KEYDOWN)
        {
            switch(event.key.keysym.sym)
            {
                case SDLK_x:
                    controllerPtr[0] |= 1;
                    break;
                case SDLK_z:
                    controllerPtr[0] |= (1<<1);
                    break;
                case SDLK_SPACE:
                    controllerPtr[0] |= (1<<2);
                    break;
                case SDLK_RETURN:
                    controllerPtr[0] |= (1<<3);
                    break;
                case SDLK_UP:
                    controllerPtr[0] |= (1<<4);
                    break;
                case SDLK_DOWN:
                    controllerPtr[0] |= (1<<5);
                    break;
                case SDLK_LEFT:
                    controllerPtr[0] |= (1<<6);
                    break;
                case SDLK_RIGHT:
                    controllerPtr[0] |= (1<<7);
                    break;
            }
        }
        else if(event.type == SDL_KEYUP)
        {
            switch(event.key.keysym.sym)
            {
                case SDLK_x:
                    controllerPtr[0] &= ~1;
                    break;
                case SDLK_z:
                    controllerPtr[0] &= ~(1<<1);
                    break;
                case SDLK_SPACE:
                    controllerPtr[0] &= ~(1<<2);
                    break;
                case SDLK_RETURN:
                    controllerPtr[0] &= ~(1<<3);
                    break;
                case SDLK_UP:
                    controllerPtr[0] &= ~(1<<4);
                    break;
                case SDLK_DOWN:
                    controllerPtr[0] &= ~(1<<5);
                    break;
                case SDLK_LEFT:
                    controllerPtr[0] &= ~(1<<6);
                    break;
                case SDLK_RIGHT:
                    controllerPtr[0] &= ~(1<<7);
                    break;
            }
        }
    }

    //Update controller keypresses
    controllerPtr[1] = 0;

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap, 256*4);
    //SDL_UpdateTexture(PTtexture, NULL, PTpixelMap, 276*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    //SDL_RenderCopy(PTrenderer, PTtexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);
    //SDL_RenderPresent(PTrenderer);
}

void close()
{
    SDL_DestroyRenderer(mainrenderer);
    SDL_DestroyRenderer(PTrenderer);
    SDL_Quit();
    return;
}

}