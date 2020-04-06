#include "gui.h"
#include "display.h"
#include "nes.h"
#include "render.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include <SDL.h>
#include <glad/glad.h>
#include <zlib.h> //crc32
#include <cstring>
#include <array>
#include <vector>
#include <iostream>
#if defined(__WIN32__)
#include <windows.h>
#include <commdlg.h>
#endif

namespace GUI {

Display mainDisplay;
SDL_Event event;
const uint8_t *kbState = SDL_GetKeyboardState(NULL);

std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT * 3> mainpixelMap;
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

int init()
{
    RENDER::init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    int expectedMenuBarSize = 19;
    mainDisplay.init(SCREEN_WIDTH*SCREEN_SCALE, SCREEN_HEIGHT*SCREEN_SCALE + expectedMenuBarSize, "plainNES");

    // Setup imGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup imGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(mainDisplay.getWindow(), mainDisplay.getContext());
    ImGui_ImplOpenGL3_Init("#version 330");

    mainDisplay.setMenuCallback(_drawmainMenuBar);

    if(initAudio()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize Audio: %s", SDL_GetError());
        return 1;
    }

    mainpixelMap.fill(0);
    mainDisplay.loadTexture(SCREEN_WIDTH, SCREEN_HEIGHT, mainpixelMap.data());

    return 0;
}

/*int initPPUWindow() {
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
}*/

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
            if(event.window.windowID == mainDisplay.getWindowID())
                quit = true;
            return;
        }
        else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            if(event.window.windowID == mainDisplay.getWindowID())
                mainDisplay.resizeImage();
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
    //if(debugPPU) {
    //    updatePPUWindow();
    //}
    if(disableAudio == false)
        updateAudio();

}

void updateMainWindow() {
    if(NES::romLoaded)
        RENDER::convertNTSC2RGB(mainpixelMap.data(), NES::getPixelMap(), SCREEN_WIDTH*SCREEN_HEIGHT*3);
    else
        mainpixelMap.fill(0);
    
    mainDisplay.loadTexture(SCREEN_WIDTH, SCREEN_HEIGHT, mainpixelMap.data());

    mainDisplay.renderFrame();
}

/*void updatePPUWindow() {
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
}*/

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

void _drawmainMenuBar() {
	static bool menu_open_file = false;
	static bool menu_quit = false;
	static bool menu_emu_run = false;
	static bool menu_emu_pause = false;
	static bool menu_emu_step = false;
	static bool menu_emu_power = false;
	static bool menu_emu_reset = false;
	static bool menu_emu_speed100 = false;
	static bool menu_emu_speedmax = false;
    //static bool menu_configInput = false;
	static bool menu_showFPS = false;
	static bool menu_debugWindow = false;
    static bool menu_get_frameInfo = false;
	//static std::map<std::string, bool> menu_open_recent;
	
    #if defined(__WIN32__)
	if(menu_open_file){ onOpenFile(); menu_open_file = false; }
    #endif
	if(menu_quit){ onQuit(); menu_quit = false; }
	if(menu_emu_run){ onEmuRun(); menu_emu_run = false; }
	if(menu_emu_pause){ onEmuPause(); menu_emu_pause = false; }
	if(menu_emu_step){ onEmuStep(); menu_emu_step = false; }
	if(menu_emu_power){ onEmuPower(); menu_emu_power = false; }
	if(menu_emu_reset){ onEmuReset(); menu_emu_reset = false; }
	if(menu_emu_speed100){ onEmuSpeed(100); menu_emu_speed100 = false; }
	if(menu_emu_speedmax){ onEmuSpeedMax(); menu_emu_speedmax = false; }
    //if(menu_configInput){ onConfigInput(); menu_configInput = false; }
	if(menu_showFPS){ onShowFPS(); menu_showFPS = false; }
	if(menu_debugWindow){ onDebugWindow(); menu_debugWindow = false; }
    if(menu_get_frameInfo){ onGetFrameInfo(); menu_get_frameInfo = false; }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(mainDisplay.getWindow());
    ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		mainDisplay.setMenuBarHeight(ImGui::GetWindowHeight());
		if (ImGui::BeginMenu("File"))
		{	
			ImGui::MenuItem("Open", NULL, &menu_open_file);
            //TODO Implement recently opened. Would need an ini type file created most likely
			/*if (ImGui::BeginMenu("Recent"))
			{
				for(item : menu_open_recent)
				{
					ImGui::MenuItem(item[0], NULL, &item[1]);
				}
				ImGui::EndMenu();
			}
			*/
			ImGui::MenuItem("Quit", NULL, &menu_quit);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Emulator"))
		{
			ImGui::MenuItem("Run", NULL, &menu_emu_run);
			ImGui::MenuItem("Pause", NULL, &menu_emu_pause);
			ImGui::MenuItem("Next Frame", NULL, &menu_emu_step);
			ImGui::Separator();
			ImGui::MenuItem("Power", NULL, &menu_emu_power);
			ImGui::MenuItem("Reset", NULL, &menu_emu_reset);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Speed"))
		{
			ImGui::MenuItem("100%", NULL, &menu_emu_speed100);
			ImGui::MenuItem("Max (No Audio)", NULL, &menu_emu_speedmax);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Options"))
		{
            //ImGui::MenuItem("Configure Input", NULL, &menu_configInput);
			ImGui::MenuItem("Show FPS", NULL, &menu_showFPS);
			ImGui::MenuItem("Debug Window", NULL, &menu_debugWindow, false);
            ImGui::MenuItem("Get Frame Info", NULL, &menu_get_frameInfo);
			ImGui::EndMenu();
		}
        if(showFPS) {
            std::string FPStext = "FPS: " + std::to_string(ImGui::GetIO().Framerate);
            ImGui::TextColored(ImColor(255,100,100), FPStext.c_str());
        }
		ImGui::EndMainMenuBar();
	}
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#if defined(__WIN32__)
void onOpenFile()
{
    char filename[ MAX_PATH ];

    OPENFILENAME ofn;
        ZeroMemory( &filename, sizeof( filename ) );
        ZeroMemory( &ofn,      sizeof( ofn ) );
        ofn.lStructSize  = sizeof( ofn );
        ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
        ofn.lpstrFilter  = "iNES ROM Files\0*.nes\0Any File\0*.*\0";
        ofn.lpstrFile    = filename;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrTitle   = "Select a ROM to load";
        ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA( &ofn ))
    {
        NES::loadROM(filename);
        NES::powerOn();
    }
}
#endif

void onQuit()
{
    quit = true;
}

void onEmuRun()
{
    NES::pause(false);
}

void onEmuPause()
{
    NES::pause(true);
}

void onEmuStep()
{
    NES::pause(true);
    NES::frameStep(true);
}

void onEmuPower()
{
    NES::powerOn();
}

void onEmuReset()
{
    NES::reset();
}

void onEmuSpeed(int pct)
{
    disableAudio = false;
}

void onEmuSpeedMax()
{
    disableAudio = true;
}

void onShowFPS()
{
    showFPS = !showFPS;
}

void onDebugWindow()
{
    //TODO get debug window working
}

void onGetFrameInfo()
{
    uint8_t *screenOutput = NES::getPixelMap();
    unsigned int crc = crc32(0L, screenOutput, 240*256);
    std::cout << "FrameNum: " << std::dec << NES::getFrameNum() << " CRC: 0x" << std::hex << crc << std::endl;
}


void close()
{
    delete &mainDisplay;
    //SDL_DestroyWindow(PPUwindow);
    SDL_CloseAudioDevice(1);
    SDL_Quit();
    return;
}

}
