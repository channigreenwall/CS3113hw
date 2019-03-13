#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glm/ext.hpp"
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    GLuint puppy1 = LoadTexture(RESOURCE_FOLDER"puppy1.jpg");
    GLuint duck1 = LoadTexture(RESOURCE_FOLDER"duck.jpg");
    GLuint kitty1 = LoadTexture(RESOURCE_FOLDER"kitty.jpg");
    //GLuint puppy1 = LoadTexture("/Users/channigreenwall/Desktop/GP-HW/HW1-GP/puppy1.jpg");
    //GLuint duck1 = LoadTexture("/Users/channigreenwall/Desktop/GP-HW/HW1-GP/duck.jpg");
    //GLuint kitty1 = LoadTexture("/Users/channigreenwall/Desktop/GP-HW/HW1-GP/kitty.jpg");
    if (puppy1 ==-1){
        std::cout<<"not working puppy"<<std::endl;
    }
    if (duck1 ==-1){
        std::cout<<"not working duck"<<std::endl;
    }
    if (kitty1 ==-1){
        std::cout<<"not working kitty"<<std::endl;
    }
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(.5f,.3f,.6f,1.0f);
        program.SetModelMatrix(modelMatrix);
        program.SetViewMatrix(viewMatrix);
        glBindTexture(GL_TEXTURE_2D, puppy1);
        float vertices[]= {-.5,-.5,.5,-.5,.5,.5,-.5,-.5,.5,.5,-.5,.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        float texCoords[]= {0,1,1,1,1,0,0,1,1,0,0,0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, duck1);
        float vertices1[]= {.5,.5,1.5,.5,1.5,1.5,.5,.5,1.5,1.5,.5,1.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, kitty1);
        float vertices2[]= {.5,-1.5,1.5,-1.5,1.5,-.5,.5,-1.5,1.5,-.5,.5,-.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
        glEnableVertexAttribArray(program.positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
