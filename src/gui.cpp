#include "gui.h"
#include "nes.h"
#include "render.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <cstring>
#include <array>
#include <vector>
#include <iostream>

namespace GUI {

SDL_Window *mainwindow;
SDL_Renderer *mainrenderer;
SDL_Texture *maintexture;
SDL_Texture *FPStextTexture;
SDL_Surface *FPStextSurface;
SDL_Color FPStextColor = {0};
TTF_Font *gFont = NULL;
SDL_Rect textRect;
char FPStext[20];
float avgFPS = 0;

SDL_Window *PPUwindow;
SDL_Renderer *PPUrenderer;
SDL_Texture *PTtexture0;
SDL_Texture *PTtexture1;
SDL_Rect rectPT0, rectPT1;
std::array<std::array<SDL_Rect, 4>, 8> PaletteRect;

SDL_Event event;
const uint8_t *kbState = SDL_GetKeyboardState(NULL);

std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT> mainpixelMap;
std::array<uint32_t, 16*8 * 16*8> PTpixelMap;

//Ratio between APU sample rate and emulator sample rate isn't a
//whole number, so using a float accounts for rounding
float rawSamplesPerSample = 1;

int volatile audio_buffer_count, audio_rb_idx;
int audio_wb_idx, audio_wb_pos;
std::vector<std::array<int16_t, AUDIO_BUFFER_SIZE>> audio_buffers;
SDL_sem * volatile audio_semaphore;

bool showFPS = false;
bool disableAudio = false;
bool debugPPU = false;

bool quit = false;
bool LctrlPressed = false;
bool RctrlPressed = false;

void setOptions(int options)
{
    showFPS = (options & DISPLAY_FPS) > 0;
    debugPPU = (options & PPU_DEBUG) > 0;
    disableAudio = (options & DISABLE_AUDIO) > 0;
}

int init()
{
    RENDER::init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if(TTF_Init() == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    if(initMainWindow()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Main Window: %s", SDL_GetError());
        return 1;
    }

    if(debugPPU) {
        if(initPPUWindow()) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Main Window: %s", SDL_GetError());
            return 1;
        }
    }

    if(initAudio()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Audio: %s", SDL_GetError());
        return 1;
    }

    gFont = TTF_OpenFont( "Roboto-Regular.ttf", 12 );
    if( gFont == NULL )
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
        return 1;
    }

    return 0;
}

int initMainWindow() {
    mainwindow = SDL_CreateWindow("plainNES",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH*SCREEN_SCALE,SCREEN_HEIGHT*SCREEN_SCALE,
                0);
    
    mainrenderer = SDL_CreateRenderer(mainwindow, -1, 0);
    SDL_RenderSetScale(mainrenderer, SCREEN_SCALE, SCREEN_SCALE);
    maintexture = SDL_CreateTexture(mainrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    for(int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; ++i) {
        mainpixelMap[i] = 0xFF000000;
    }

    FPStextColor.r = 255;
    FPStextColor.g = 0;
    FPStextColor.b = 0;

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap.data(), SCREEN_WIDTH*4);
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
                paletteByte = NES::getPalette(0x3F00);
            }
            else {
                paletteByte = NES::getPalette(0x3F00 + i*4 + j);
            }
            RENDER::convertNTSC2ARGB(&paletteARGB, &paletteByte, 1);
            SDL_SetRenderDrawColor(PPUrenderer, (paletteARGB >> 16) & 0xFF, (paletteARGB >> 8) & 0xFF, paletteARGB & 0xFF, 255);
            SDL_RenderFillRect(PPUrenderer, &PaletteRect[i][j]);
        }
    }
    
    SDL_RenderPresent(PPUrenderer);

    return 0;
}

int initAudio() {
    audio_rb_idx = 0;
    audio_wb_idx = 0;
    audio_wb_pos = 0;

    rawSamplesPerSample = (float)NES::APU_CLOCK_RATE/AUDIO_SAMPLE_RATE;

    //Create audio buffer
    int32_t sample_latency = WANTED_AUDIO_LATENCY_MS * AUDIO_SAMPLE_RATE * AUDIO_CHANNELS / 1000;
    audio_buffer_count = sample_latency / AUDIO_BUFFER_SIZE;
    if(audio_buffer_count < 2)
        audio_buffer_count = 2;
    audio_buffers.resize(audio_buffer_count);

    //Create SDL semaphore
    audio_semaphore = SDL_CreateSemaphore(audio_buffer_count-1);

    SDL_AudioSpec wantedSpec;
    wantedSpec.freq = AUDIO_SAMPLE_RATE;
    wantedSpec.format = AUDIO_S16SYS;
    wantedSpec.channels = AUDIO_CHANNELS;
    wantedSpec.samples = AUDIO_BUFFER_SIZE;
    wantedSpec.callback = fill_audio_buffer;

    if(SDL_OpenAudio(&wantedSpec, 0) < 0)
        return 1;
    SDL_PauseAudio(0);
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
                    NES::controller_state[0] |= 1;
                    break;
                case SDLK_z:
                    NES::controller_state[0] |= (1<<1);
                    break;
                case SDLK_SPACE:
                    NES::controller_state[0] |= (1<<2);
                    break;
                case SDLK_RETURN:
                    NES::controller_state[0] |= (1<<3);
                    break;
                case SDLK_UP:
                    NES::controller_state[0] |= (1<<4);
                    break;
                case SDLK_DOWN:
                    NES::controller_state[0] |= (1<<5);
                    break;
                case SDLK_LEFT:
                    NES::controller_state[0] |= (1<<6);
                    break;
                case SDLK_RIGHT:
                    NES::controller_state[0] |= (1<<7);
                    break;
                case SDLK_r:
                    if(LctrlPressed || RctrlPressed) {
                        NES::reset();
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
                    NES::controller_state[0] &= ~1;
                    break;
                case SDLK_z:
                    NES::controller_state[0] &= ~(1<<1);
                    break;
                case SDLK_SPACE:
                    NES::controller_state[0] &= ~(1<<2);
                    break;
                case SDLK_RETURN:
                    NES::controller_state[0] &= ~(1<<3);
                    break;
                case SDLK_UP:
                    NES::controller_state[0] &= ~(1<<4);
                    break;
                case SDLK_DOWN:
                    NES::controller_state[0] &= ~(1<<5);
                    break;
                case SDLK_LEFT:
                    NES::controller_state[0] &= ~(1<<6);
                    break;
                case SDLK_RIGHT:
                    NES::controller_state[0] &= ~(1<<7);
                    break;
            }
        }
    }

    //Update controller 2 keypresses
    NES::controller_state[1] = 0;

    updateMainWindow();
    if(debugPPU) {
        updatePPUWindow();
    }
    if(disableAudio == false)
        updateAudio();

}

void updateMainWindow() {
    RENDER::convertNTSC2ARGB(mainpixelMap.data(), NES::getPixelMap(), SCREEN_WIDTH*SCREEN_HEIGHT);

    SDL_UpdateTexture(maintexture, NULL, mainpixelMap.data(), SCREEN_WIDTH*4);
    SDL_RenderCopy(mainrenderer, maintexture, NULL, NULL);

    //FPS text
    if(showFPS) {
        snprintf(FPStext, sizeof(FPStext), "%.2f", avgFPS);
        FPStextSurface = TTF_RenderText_Blended( gFont, FPStext, FPStextColor );
        textRect.w = FPStextSurface->w;
        textRect.h = FPStextSurface->h;
        textRect.x = SCREEN_WIDTH-textRect.w;
        textRect.y = 0;
        FPStextTexture = SDL_CreateTextureFromSurface(mainrenderer, FPStextSurface);
        SDL_RenderCopy(mainrenderer, FPStextTexture, NULL, &textRect);
    }

    SDL_RenderPresent(mainrenderer);
}

void updatePPUWindow() {
    std::array<std::array<uint8_t, 16*16*64>, 2> PTarrays = NES::getPatternTableBuffers();
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
                paletteByte = NES::getPalette(0x3F00);
            }
            else {
                paletteByte = NES::getPalette(0x3F00 + i*4 + j);
            }
            RENDER::convertNTSC2ARGB(&paletteARGB, &paletteByte, 1);
            SDL_SetRenderDrawColor(PPUrenderer, (paletteARGB >> 16) & 0xFF, (paletteARGB >> 8) & 0xFF, paletteARGB & 0xFF, 255);
            SDL_RenderFillRect(PPUrenderer, &PaletteRect[i][j]);
        }
    }

    SDL_RenderPresent(PPUrenderer);
}

void updateAudio() {
    //Currently downsample using nearest neighbor method
    //TODO: Look into using FIR filter or similar
    int size = NES::rawAudio.writeIdx - NES::rawAudio.readIdx;
    if(size < 0) size += NES::rawAudio.buffer.size();

    float currentRawBufferIdx = NES::rawAudio.readIdx;
    while(size > 0) {
        audio_buffers[audio_wb_idx][audio_wb_pos] = (int16_t)((NES::rawAudio.buffer[(unsigned int)currentRawBufferIdx]*2.0f - 1.0f) * 0xFFF);
        currentRawBufferIdx += rawSamplesPerSample;
        if(currentRawBufferIdx >= NES::rawAudio.buffer.size()) currentRawBufferIdx -= (float)NES::rawAudio.buffer.size();
        size -= rawSamplesPerSample;
        ++audio_wb_pos;

        if(audio_wb_pos >= AUDIO_BUFFER_SIZE) {
            //If current buffer full, move to next one
            //If all buffers full, wait
            audio_wb_pos = 0;
            audio_wb_idx = (audio_wb_idx + 1) % audio_buffer_count;
            SDL_SemWait(audio_semaphore);
        }
    }
    NES::rawAudio.readIdx = currentRawBufferIdx;
}

void fill_audio_buffer(void *user_data, uint8_t *out, int byte_count) {
    if(SDL_SemValue(audio_semaphore) < audio_buffers.size() - 1) {
        //At least one full buffer
        if(byte_count != AUDIO_BUFFER_SIZE*sizeof(int16_t))
            std::cout << "Wrong buffer size: " << byte_count << " vs " << AUDIO_BUFFER_SIZE*sizeof(int16_t) << std::endl;
        memcpy(out, audio_buffers[audio_rb_idx].data(), AUDIO_BUFFER_SIZE*sizeof(int16_t));
        audio_rb_idx = (audio_rb_idx+1) % audio_buffers.size();
        SDL_SemPost(audio_semaphore);
    }
    else {
        //No buffers full. Just output silence
        memset(out, 0, byte_count);
    }
}

void close()
{
    SDL_DestroyWindow(mainwindow);
    SDL_DestroyWindow(PPUwindow);
    SDL_CloseAudioDevice(1);
    SDL_Quit();
    return;
}

}