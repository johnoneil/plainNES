#include "gui.h"
#include "render.h"
#include "ppu.h"
#include "io.h"
#include "cpu.h"
#include <SDL.h>
#include <cstring>
#include <array>

namespace GUI {

#define SCREEN_SCALE 2

SDL_Window *mainwindow;
SDL_Window *PPUwindow;
SDL_Renderer *mainrenderer;
SDL_Renderer *PPUrenderer;
SDL_Texture *maintexture;
SDL_Texture *PTtexture0;
SDL_Texture *PTtexture1;
SDL_Rect rectPT0, rectPT1;
std::array<std::array<SDL_Rect, 4>, 8> PaletteRect;
SDL_Event event;
const uint8_t *kbState = SDL_GetKeyboardState(NULL);

std::array<uint32_t, 256 * 240> mainpixelMap;
std::array<uint32_t, 16*8 * 16*8> PTpixelMap;
bool quit = false;
bool LctrlPressed = false;
bool RctrlPressed = false;

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

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap.data(), 256*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);

    return 0;
}

int initPPUWindow() {
    PPUwindow = SDL_CreateWindow("plainNES - PPU Debugging",
                50, 50,
                (32*8 + 62 + 4 + 4)*SCREEN_SCALE,(16*8 + 4)*SCREEN_SCALE,
                0);
    
    
    PPUrenderer = SDL_CreateRenderer(PPUwindow, -1, 0);
    SDL_RenderSetScale(PPUrenderer, SCREEN_SCALE, SCREEN_SCALE);
    SDL_SetRenderDrawColor(PPUrenderer, 157, 161, 170, 255); 
    SDL_RenderClear(PPUrenderer); //Fill with grey color by default

    //Setup Pattern Table Textures
    PTtexture0 = SDL_CreateTexture(PPUrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 16*8, 16*8);
    PTtexture1 = SDL_CreateTexture(PPUrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 16*8, 16*8);
    rectPT0.x = 2;
    rectPT0.y = 2;
    rectPT0.h = 16*8;
    rectPT0.w = 16*8;
    rectPT1.x = 16*8 + 4;
    rectPT1.y = 2;
    rectPT1.h = 16*8;
    rectPT1.w = 16*8;

    for(int i=0; i<16*8 * 16*8; ++i) {
        PTpixelMap[i] = 0xFF000000;
    }

    SDL_UpdateTexture(PTtexture0, NULL, PTpixelMap.data(), 16*8*4);
    SDL_UpdateTexture(PTtexture1, NULL, PTpixelMap.data(), 16*8*4);
    SDL_RenderCopy(PPUrenderer, PTtexture0, NULL, &rectPT0);
    SDL_RenderCopy(PPUrenderer, PTtexture1, NULL, &rectPT1);

    //Setup Palette Textures
    //Each palette is 14x14 with 2 pixel borders for a total of 62x
    for(int i=0; i<8; ++i) {
        for(int j=0; j<4; ++j) {
            PaletteRect[i][j].x = 262 + j*16;
            PaletteRect[i][j].y = 2 + i*16;
            PaletteRect[i][j].h = 14;
            PaletteRect[i][j].w = 14;
        }
    }
    for(int i=0; i<8; ++i) {
        for(int j=0; j<4; ++j) {
            uint32_t paletteARGB;
            uint8_t paletteByte;
            if(j == 0) {
                paletteByte = PPU::getPalette(0x3F00);
            }
            else {
                paletteByte = PPU::getPalette(0x3F00 + i*4 + j);
            }
            RENDER::convertNTSC2ARGB(&paletteARGB, &paletteByte, 1);
            SDL_SetRenderDrawColor(PPUrenderer, (paletteARGB >> 16) & 0xFF, (paletteARGB >> 8) & 0xFF, paletteARGB & 0xFF, 255);
            SDL_RenderFillRect(PPUrenderer, &PaletteRect[i][j]);
        }
    }
    
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
                case SDLK_LCTRL:
                    LctrlPressed = true;
                    break;
                case SDLK_RCTRL:
                    RctrlPressed = true;
                    break;
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
                case SDLK_r:
                    if(LctrlPressed || RctrlPressed) {
                        CPU::reset();
                    }
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
                case SDLK_LCTRL:
                    LctrlPressed = false;
                    break;
                case SDLK_RCTRL:
                    RctrlPressed = false;
                    break;
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
    RENDER::convertNTSC2ARGB(mainpixelMap.data(), PPU::getPixelMap(), 256*240);

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap.data(), 256*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);
    SDL_RenderPresent(mainrenderer);
}

void updatePPUWindow() {
    std::array<std::array<uint8_t, 16*16*64>, 2> PTarrays = PPU::getPatternTableBuffers();
    RENDER::convertNTSC2ARGB(PTpixelMap.data(), PTarrays[0].data(), PTarrays[0].size());
    SDL_UpdateTexture(PTtexture0, NULL, PTpixelMap.data(), 16*8*4);
    RENDER::convertNTSC2ARGB(PTpixelMap.data(), PTarrays[1].data(), PTarrays[1].size());
    SDL_UpdateTexture(PTtexture1, NULL, PTpixelMap.data(), 16*8*4);

    SDL_RenderCopy(PPUrenderer, PTtexture0, NULL, &rectPT0);
    SDL_RenderCopy(PPUrenderer, PTtexture1, NULL, &rectPT1);

    for(int i=0; i<8; ++i) {
        for(int j=0; j<4; ++j) {
            uint32_t paletteARGB;
            uint8_t paletteByte;
            if(j == 0) {
                paletteByte = PPU::getPalette(0x3F00);
            }
            else {
                paletteByte = PPU::getPalette(0x3F00 + i*4 + j);
            }
            RENDER::convertNTSC2ARGB(&paletteARGB, &paletteByte, 1);
            SDL_SetRenderDrawColor(PPUrenderer, (paletteARGB >> 16) & 0xFF, (paletteARGB >> 8) & 0xFF, paletteARGB & 0xFF, 255);
            SDL_RenderFillRect(PPUrenderer, &PaletteRect[i][j]);
        }
    }

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