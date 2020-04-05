#include "display.h"
#include "shader.h"
#include <glad/glad.h>
#include <SDL.h>
#include <iostream>

Display::Display()
{

}

Display::Display(int width, int height, const char* title)
{
    init(width, height, title);
}

Display::~Display()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}

void Display::init(int width, int height, const char* title)
{
    window = SDL_CreateWindow(title,
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                width, height,
                SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    gl_context = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    shader.init();

    //Setup vertex data and buffers and configure
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Setup texture info
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	// set texture wrapping to GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {0,0,0,1}; //Black border
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    resizeImage();
}

void Display::loadTexture(int width, int height, uint8_t *data)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    textureHeight = height;
    textureWidth = width;
}

void Display::renderFrame()
{
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, texture);

    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    if(menuCallbackFun) //Skips if NULL
        menuCallbackFun();

    SDL_GL_SwapWindow(window);
}

void Display::resizeImage()
{
    int winWidth, winHeight, viewWidth, viewHeight, viewX, viewY;
    SDL_GL_GetDrawableSize(window, &winWidth, &winHeight);

    if((float)winWidth/(winHeight - menuBarHeight) > (float)textureWidth/textureHeight) { //Usable window wider than game display
        viewHeight = winHeight - menuBarHeight;
        viewY = 0;
        viewWidth = (float)textureWidth/textureHeight * viewHeight;
        viewX = (winWidth - viewWidth) / 2;
    }
    else { //Window taller than game display
        viewWidth = winWidth;
        viewX = 0;
        viewHeight = (float)textureHeight/textureWidth * viewWidth;
        viewY = (winHeight - menuBarHeight - viewHeight) / 2;
    }
    glViewport(viewX, viewY, viewWidth, viewHeight);
}

uint32_t Display::getWindowID()
{
    return SDL_GetWindowID(window);
}

SDL_Window *Display::getWindow()
{
    return window;
}

SDL_GLContext Display::getContext()
{
    return gl_context;
}

void Display::setMenuBarHeight(int height)
{
    menuBarHeight = height;
}

void Display::setMenuCallback(std::function<void()> callbackFun)
{
    menuCallbackFun = callbackFun;
}