#pragma once

#include <SDL2/SDL.h>
#include <array>
#include "shader.h"

class Shader;

class Display
{
public:
    Display();
    Display(int width, int height, const char* title,
        const char* vertexPath, const char* fragmentPath);
    ~Display();
    void init(int width, int height, const char* title,
        const char* vertexPath, const char* fragmentPath);
    void loadTexture(int width, int height, uint32_t *data);
    void renderFrame();

private:
    int textureWidth, textureHeight;
    SDL_GLContext gl_context;
    SDL_Window *window;
    Shader shader;

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

    void resizeImage();

};