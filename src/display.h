#pragma once

#include <SDL.h>
#include <array>
#include <functional>
#include "shader.h"

class Shader;

class Display
{
public:
    Display();
    Display(int width, int height, const char* title);
    ~Display();
    void init(int width, int height, const char* title);
    
    //Load new texture into buffer
    void loadTexture(int width, int height, uint8_t *data);

    //Render texture and menu if enabled
    void renderFrame();

    //Resizes viewport to match window
    void resizeImage();

    uint32_t getWindowID();
    SDL_Window *getWindow();
    SDL_GLContext getContext();
    void setMenuBarHeight(int height);


    //Set menu callback. If NULL, no menu is generated
    void setMenuCallback(std::function<void()> callbackFun);

private:
    int textureWidth = 256, textureHeight = 240, menuBarHeight = 19;
    SDL_GLContext gl_context;
    SDL_Window *window;
    Shader shader;

    std::function<void()> menuCallbackFun;

    unsigned int VBO, VAO, EBO, texture;

    float vertices[20] = {
         //positions          // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 0.0f, // top right
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 1.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   0.0f, 0.0f  // top left 
    };
    unsigned int indices[6] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    void drawMenu();
};