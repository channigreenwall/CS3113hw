#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <math.h>
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glm/ext.hpp"
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define pie 3.14159265

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
struct Entity{ //store all info for every object
    float flipx = 1; //when colliding with paddle flips x axis val
    float x;
    float y;
    float maxy = .88f; //roof of game
    float maxx = 1.7f; //walls of game
    //float rotation;
    GLuint textureID; //pointer to texture
    float width; //width/heights of objects themselves
    float height;
    float velocity; //speed an entity is going
    //float direction_x = 1;
    float direction_y = 1; //flip for y axis
    float angle=45; //angle ball is going
    float paddle; //tells program weather paddle or ball (ball is 0)
    int counterleft=0; //score
    int counterright=0;
    float vertices[12] = { -0.05f, -0.05f, -0.05f, .05f, .05f, .05f, 0.05f, 0.05f, -0.05f, -0.05f, .05f, -0.05f }; //intial x/y coords
    void movepaddle(float elapsed, float direction){
        float stopper = 1; //if y coord of paddle is too large or to small it will stop paddel from moving anymore
        if(y > (maxy - .4f) && direction == 1)
        {
            stopper = 0;
        }
        else if(y <= (-1*(maxy - .4f)) && direction == -1)
        {
            stopper = 0;
        }
        float temp = y+(velocity*direction*elapsed * stopper);
        //std::cout << "Left or rigth" << std::endl;
        y = temp;
        //moving paddle with velocity&direction
    }
    glm::mat4 viewMatrix = glm::mat4(1.0f); //where vertices are stored
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    Entity(float x1,float y1,float width1,float height1, GLuint text1, float vel1, float isPaddle){
        x=x1;
        y=y1;
        width=width1;
        height=height1;
        textureID=text1;
        velocity=vel1;
        paddle = isPaddle;
    }
    void collision(Entity* e){
        //what is causing collision
        bool rightpadde = e->x >= 0; //check to see which we are colliding with, if x>0 not left
        bool leftpaddle = e->x <= 0;//no way right paddle
        //collision of any two boxes
        float  paddleroof = e->y + (e->height / 2.0f); //top middle
        float paddlefloor = e->y - (e->height / 2.0f);//bottom moddle
        float paddleleft = e->x - (e->width / 2.0f);
        float paddleright = e->x + (e->width / 2);
        
        float ballroof = y + (height / 2);//top point of ball
        float ballfloor = y - (height / 2);
        float ballright = x + (width/ 2);
        float ballleft = x - (width / 2);
        bool ballxpaddle = ((ballright >= paddleleft && ballleft <= paddleright) || (ballright >= paddleleft && ballleft < paddleleft));//
        //1st, if ball is on edge and greater left/right side then the paddle ie then fall off
        //2nd, if ball hits paddle directly on roof or floor
        //3rd, like 1st but
        //bool undertest = (ballroof >= paddlefloor && ballxpaddle && paddleroof > ballroof && paddlefloor > ballfloor);
        //bool overtest = (ballfloor <= paddleroof && ballxpaddle && paddlefloor <= ballfloor && paddleroof < ballroof);
        bool inrange = (ballfloor <= paddleroof && ballxpaddle && ballroof >= paddlefloor);
        //checks if the floor of ball is less than roof of paddle and if roof of ball is greater than floor, so ball is on y axis
        if (leftpaddle && inrange) //move right if u collide
        {
            flipx = 1.0f;
        }
        if (rightpadde && inrange) //
        {
            flipx = -1.0f;
        }
        if(paddle == 0)
        {
            //std::cout<< "flipy" << std::endl;
            if(y >= maxy) //flip in y axis, hits rooof so flips downward
            {
                direction_y = -1.0f;
            }
            else if(y<= (-1*maxy))//floor flip upward
            {
                direction_y = 1.0f;
            }
        }
        if(paddle == 0 && x <= (-1*maxx)) //left and right walls
        {
            counterright+=1;
            std::cout << "right player won " <<counterright<<" games"<< std::endl;
            std::cout << "left player won " <<counterleft<<" games"<< std::endl;
            x = 0;
            y = 0;
        }
        else if (paddle == 0 && x >= (maxx))
        {
            counterleft+=1;
            std::cout << "left player won " <<counterleft<<" games"<< std::endl;
            std::cout << "right player won " <<counterright<<" games"<< std::endl;
            x = 0;
            y = 0;
        }
        
        
    }
    
    
    void Draw(ShaderProgram &program){
        program.SetModelMatrix(modelMatrix);
        program.SetViewMatrix(viewMatrix);
        //std::cout << vertices[0] << std::endl;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        float texCoords[]= {0,1,1,1,1,0,0,1,1,0,0,0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    void update(float elapsed){ //keeps game and computer in tandem
        double xdir = cos(angle*pie/180);
        int stopper = 1;
        float ydir = 0.0f;
        if(paddle == 0){
            x += elapsed*velocity * flipx *xdir;
            ydir = sin(angle*pie/180);
        }
        y += elapsed * velocity * ydir * direction_y *stopper;
        //y += elapsed*velocity;
        
        vertices[0]= (x-(width/2));
        vertices[1] =(y-(height/2));
        vertices[2] = (x+(width/2));
        vertices[3] = (y-(height/2));
        vertices[4] = (x+(width/2));
        vertices[5] =(y+(height/2));
        vertices[6] = (x-(width/2));
        vertices[7] =(y-(height/2));
        vertices[8] = (x+(width/2));
        vertices[9] = (y+(height/2));
        vertices[10] = (x-(width/2));
        vertices[11] = (y+(height/2));
        //vertices = verts;
    }
    
};

int main(int argc, char *argv[])
{
    float lastFrameTicks = 0.0f;
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
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    Entity ent1(-1,0,.25,.25,puppy1,1,0);
    Entity entright(2,0,1,1,duck1,.5,1);
    Entity entleft(-2,0,1,1,kitty1,.5,1);
    SDL_Event event;
    bool done = false;
    while (!done) {
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks; //number of frames that passed by
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        if (keys[SDL_SCANCODE_W] && keys[SDL_SCANCODE_UP])
        {
            entleft.movepaddle(elapsed, 1);
            entright.movepaddle(elapsed, 1);
        }
        else if (keys[SDL_SCANCODE_S] && keys[SDL_SCANCODE_DOWN])
        {
            entleft.movepaddle(elapsed, -1);
            entright.movepaddle(elapsed, -1);
        }
        else if (keys[SDL_SCANCODE_W])
        {
            entleft.movepaddle(elapsed, 1);
        }
        else if (keys[SDL_SCANCODE_UP])
        {
            entright.movepaddle(elapsed, 1);
        }
        else if (keys[SDL_SCANCODE_S])
        {
            entleft.movepaddle(elapsed, -1);
        }
        else if (keys[SDL_SCANCODE_DOWN])
        {
            entright.movepaddle(elapsed, -1);
        }
        
        lastFrameTicks = ticks;
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(.5f,.3f,.6f,1.0f);
        ent1.collision(&entright);
        ent1.collision(&entleft);
        
        ent1.update(elapsed);
        entright.update(elapsed);
        entleft.update(elapsed);
        ent1.Draw(program);
        entright.Draw(program);
        entleft.Draw(program);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

