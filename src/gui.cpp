#include "gui.h"
#include "render.h"
#include "ppu.h"
#include "io.h"
#include <SDL.h>
#undef main
#include <cstring>
#include <array>

namespace GUI {

#define SCREEN_SCALE 2

SDL_Window *mainwindow;
SDL_Window *PPUwindow;
SDL_Renderer *mainrenderer;
SDL_Renderer *PPUrenderer;
SDL_Texture *maintexture;
SDL_Texture *PPUtexture0;
SDL_Texture *PPUtexture1;
SDL_Rect rectPT0, rectPT1;
SDL_Event event;
const uint8_t *kbState = SDL_GetKeyboardState(NULL);

uint32_t mainpixelMap[256 * 240];
uint32_t PPUpixelMap[16*8 * 16*8];
bool quit = false;

int init()
{
    RENDER::init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if(initMainWindow()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Main Window: %s", SDL_GetError());
        return 1;
    }

    if(initPPUWindow()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Main Window: %s", SDL_GetError());
        return 1;
    }

    return 0;
}

int initMainWindow() {
    mainwindow = SDL_CreateWindow("plainNES",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                256*SCREEN_SCALE,240*SCREEN_SCALE,
                0);
    
    mainrenderer = SDL_CreateRenderer(mainwindow, -1, 0);
    SDL_RenderSetScale(mainrenderer, SCREEN_SCALE, SCREEN_SCALE);
    maintexture = SDL_CreateTexture(mainrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    for(int i=0; i<256*240; ++i) {
        mainpixelMap[i] = 0xFF000000;
    }

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap, 256*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);

    return 0;
}

int initPPUWindow() {
    PPUwindow = SDL_CreateWindow("plainNES - PPU Debugging",
                50, 50,
                32*8*SCREEN_SCALE,16*8*SCREEN_SCALE,
                0);
    
    PPUrenderer = SDL_CreateRenderer(PPUwindow, -1, 0);
    SDL_RenderSetScale(PPUrenderer, SCREEN_SCALE, SCREEN_SCALE);
    PPUtexture0 = SDL_CreateTexture(PPUrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 16*8, 16*8);
    PPUtexture1 = SDL_CreateTexture(PPUrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 16*8, 16*8);
    rectPT0.x = 0;
    rectPT0.y = 0;
    rectPT0.h = 16*8;
    rectPT0.w = 16*8;
    rectPT1.x = 16*8;
    rectPT1.y = 0;
    rectPT1.h = 16*8;
    rectPT1.w = 16*8;

    for(int i=0; i<16*8 * 16*8; ++i) {
        PPUpixelMap[i] = 0xFF000000;
    }

    SDL_UpdateTexture(PPUtexture0, NULL, PPUpixelMap, 16*8*4);
    SDL_UpdateTexture(PPUtexture1, NULL, PPUpixelMap, 16*8*4);
    SDL_RenderCopy(PPUrenderer, PPUtexture0, NULL, &rectPT0);
    SDL_RenderCopy(PPUrenderer, PPUtexture1, NULL, &rectPT1);
    SDL_RenderPresent(PPUrenderer);

    return 0;
}

void update()
{  
    while( SDL_PollEvent(&event) != 0) {
        if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            quit = true;
            return;
        }
        else if(event.type == SDL_KEYDOWN)
        {
            switch(event.key.keysym.sym)
            {
                case SDLK_x:
                    IO::controller_state[0] |= 1;
                    break;
                case SDLK_z:
                    IO::controller_state[0] |= (1<<1);
                    break;
                case SDLK_SPACE:
                    IO::controller_state[0] |= (1<<2);
                    break;
                case SDLK_RETURN:
                    IO::controller_state[0] |= (1<<3);
                    break;
                case SDLK_UP:
                    IO::controller_state[0] |= (1<<4);
                    break;
                case SDLK_DOWN:
                    IO::controller_state[0] |= (1<<5);
                    break;
                case SDLK_LEFT:
                    IO::controller_state[0] |= (1<<6);
                    break;
                case SDLK_RIGHT:
                    IO::controller_state[0] |= (1<<7);
                    break;
                case SDLK_ESCAPE:
                    quit = true;
                    return;
            }
        }
        else if(event.type == SDL_KEYUP)
        {
            switch(event.key.keysym.sym)
            {
                case SDLK_x:
                    IO::controller_state[0] &= ~1;
                    break;
                case SDLK_z:
                    IO::controller_state[0] &= ~(1<<1);
                    break;
                case SDLK_SPACE:
                    IO::controller_state[0] &= ~(1<<2);
                    break;
                case SDLK_RETURN:
                    IO::controller_state[0] &= ~(1<<3);
                    break;
                case SDLK_UP:
                    IO::controller_state[0] &= ~(1<<4);
                    break;
                case SDLK_DOWN:
                    IO::controller_state[0] &= ~(1<<5);
                    break;
                case SDLK_LEFT:
                    IO::controller_state[0] &= ~(1<<6);
                    break;
                case SDLK_RIGHT:
                    IO::controller_state[0] &= ~(1<<7);
                    break;
            }
        }
    }

    //Update controller 2 keypresses
    IO::controller_state[1] = 0;

    updateMainWindow();
    updatePPUWindow();

}

void updateMainWindow() {
    RENDER::convertNTSC2ARGB(mainpixelMap, PPU::getPixelMap(), 256*240);

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap, 256*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);
}

void updatePPUWindow() {
    std::array<std::array<uint8_t, 16*16*64>, 2> PTarrays = PPU::getPatternTableBuffers();
    RENDER::convertNTSC2ARGB(PPUpixelMap, PTarrays[0].data(), PTarrays[0].size());
    SDL_UpdateTexture(PPUtexture0, NULL, PPUpixelMap, 16*8*4);
    RENDER::convertNTSC2ARGB(PPUpixelMap, PTarrays[1].data(), PTarrays[1].size());
    SDL_UpdateTexture(PPUtexture1, NULL, PPUpixelMap, 16*8*4);

    SDL_RenderCopy(PPUrenderer, PPUtexture0, NULL, &rectPT0);
    SDL_RenderCopy(PPUrenderer, PPUtexture1, NULL, &rectPT1);
    SDL_RenderPresent(PPUrenderer);
}

void close()
{
    SDL_DestroyRenderer(mainrenderer);
    SDL_DestroyRenderer(PPUrenderer);
    SDL_Quit();
    return;
}

}