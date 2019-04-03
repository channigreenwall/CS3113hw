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
#include <vector>
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
        //assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
} //gives you GLUint to use
class SheetSprite { //takes x & y coords of individual texture on sheet
    public:
        float size;
        unsigned int textureID;
        float u; //x coordinate on texture itself
        float v; //y coordinate on texture itself
        float width; //width of texture (ex: letter)
        float height; //height of texture (ex: letter)
        SheetSprite(){};
        SheetSprite(unsigned int textureID1, float u1, float v1, float width1, float height1, float size1)
    {
        textureID = textureID1;
        u = u1;
        v = v1;
        width = width1;
        height = height1;
        size = size1;
    };

    //this function is for ordered sprite sheets (in our case letter sheet)
    void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY) {
        float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
        //because in order if we take mod we get x val, if we divide we get y
        float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
        float spriteWidth = 1.0/(float)spriteCountX;
        float spriteHeight = 1.0/(float)spriteCountY;
        float texCoords[] = { //draws box for each letter (total 13 boxes for spaceinvaders) from two triangles
            u, v+spriteHeight, //top left hand
            u+spriteWidth, v, //bottom right
            u, v, //bottom left
            u+spriteWidth, v, //bottom right
            u, v+spriteHeight,//top left hand
            u+spriteWidth, v+spriteHeight//top right
        };
        float val[12]={-.5f,-.5f,.5f,.5f,-.5f,.5f,.5f,.5f,-.5f,-.5f,.5f,-.5f}; //points of entire drawing board for game space for 1 entity ie box for 1 letter
        Draw(program,texCoords,val); //val where to draw, texcoords what to draw
        // draw this data
    }
    
    void Draw(ShaderProgram program,float* texCoords,float* vertCoords){
        glBindTexture(GL_TEXTURE_2D, textureID);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertCoords);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    void DrawSpriteUnOrder(ShaderProgram &program) { //call this function when you have unordered sprite sheet like the pictures sprite sheet
        glBindTexture(GL_TEXTURE_2D, textureID);
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        float aspect = width / height;
        float vertices[] = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size ,
            0.5f * size * aspect, -0.5f * size};
        // draw our arrays
        Draw(program,texCoords,vertices);
    }
};

struct Entity{ //store all info for every object
    bool dead = false; //keeping track of weather object should be drawn or not
    int xdir = 1; //aliens start going right, when they hit invisible object on right switch to left (so they move back and forth)
    float x;
    float y;
    GLuint textureID; //pointer to spritesheet or lettersheet
    float width; //width/heights of objects themselves
    float height;
    float velocity; //speed an entity is going
    float id; //tells program weather alien(0), left/right wall(1), ship(2), bullet(4)
    SheetSprite sprite;
    glm::mat4 viewMatrix = glm::mat4(1.0f); //sets up how camera is looking at game
    glm::mat4 modelMatrix = glm::mat4(1.0f); //stores the transformations for the positions of objects
    //constructor
    Entity(float x1,float y1,float width1,float height1, GLuint text1, float vel1, float identification){
        x=x1;
        y=y1;
        width=width1;
        height=height1;
        textureID=text1;
        velocity=vel1;
        id = identification;
        sprite=SheetSprite(text1,(425.0f / 1024.0f), (468.0f / 1024.0f), (93.0f / 1024.0f), (84.0f /1024.0f), 0.2f);
    }
    
    Entity(float x1,float y1,float width1,float height1, GLuint text1){
        x=x1;
        y=y1;
        width=width1;
        height=height1;
        textureID=text1;
        viewMatrix = glm::mat4(1.0f); //where vertices are stored
        modelMatrix = glm::mat4(1.0f);
    }
    
    bool collision(Entity* e){
        //checking me vs u collision
        //ball is my left right height ect
        //e is your left right height ect
        float  paddleroof = e->y + (e->height / 2.0f); //top middle of object checking
        float paddlefloor = e->y - (e->height / 2.0f);//bottom middle of object checking
        float paddleleft = e->x - (e->width / 2.0f); //left side of object being checked
        float paddleright = e->x + (e->width / 2);
        float ballroof = y + (height / 2);//top point of where you currently are
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
        return inrange;
    }
    void Draw(ShaderProgram &program){
        if (!dead)
        {
            modelMatrix = glm::scale(modelMatrix, glm::vec3(width, height, 1.0f));
            program.SetModelMatrix(modelMatrix);
            program.SetViewMatrix(viewMatrix);
            sprite.DrawSpriteUnOrder(program);
        }
    }
    void update(float elapsed){ //keeps game and computer in tandem
        if (!dead)
        {
            if (id == 0 || id == 2)
            {
                x += elapsed * velocity;
            }
            
            if (id == 4)
            {
                y = y + elapsed * velocity;
            }
            //floa        }
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
        }
    }
    
};

struct text{
    float text_x=0;
    float text_y=0;
    float text_width=2.5;
    float text_height=1.5;
    float fontsize=.3;
    std::string textval;
    SheetSprite* letter_and_spriteholder[13]; //entities created to hold sprite images or text images of size 13
    //GLuint spritesheet; //stores entire sprite sheet
    GLuint lettersheet; //stores entire letter sheet
    glm::mat4 modelMatrix = glm::mat4(1.0f); //translate each vertice so it will be placed correctly
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    void place_text_ongame(){
        int counterx=0; //counter to loop through array as we add new vals
        //float space_between_letters= (text_x + text_width) / 13;
        //ax is x coord on the game screen where letter will be placed
        //ax starts at left most side of letter box, and we do that for as long as we are less than the right side ie still in the box, every time we increment i by the games text box/13 so each character has equal space
        for (float ax = text_x + -(text_width / 2); ax < text_x + ((text_width) / 2); ax += ((text_x + text_width) / 13)){
            //std::cout << ax << std::endl;
            letter_and_spriteholder[counterx] = new SheetSprite(lettersheet,ax,.2,.2,.25,512.0f); //creating the space for the entity itself so it has somewhereto go, next line gets it
            //16 by 16 grid
            //(text_x_coords(text[counterx]), text_y_coords(text[counterx]), (1.0f/16.0f), (1.0f/16.0f), 1.0f, 1.0f); //changing text coordinates so computer knows where to get image from on sheet
            counterx++;
        }
    }
    void draw(ShaderProgram program){
        int counterx = 0;
        program.SetViewMatrix(viewMatrix);
        for(float ax = text_x + -(text_width / 2); ax < text_x + ((text_width) / 2); ax += ((text_x + text_width) / 13)){
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(ax,0.0f,1.0f));
            modelMatrix = glm::scale(modelMatrix, glm::vec3(fontsize,fontsize,1.0f));
            program.SetModelMatrix(modelMatrix);
            letter_and_spriteholder[counterx]->DrawSpriteSheetSprite(program, (int)(textval[counterx]), 16, 16);
            counterx ++;
        }
    }
};
struct bullethell
{
    GLuint tedxt;
    bullethell(GLuint texid)
    {
        tedxt = texid;
    };
    std::vector<Entity> bullets;
    void shoot(float x)
    {
        //Entity myEEnt(f,);
        if (bullets.size() < 1)
        {
            Entity myEntity(x, -.6f, .1, .1, tedxt, .2, 4);
            myEntity.sprite = SheetSprite(tedxt, 854.0f / 1024.0f, 639.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 1);
            bullets.push_back(myEntity);
        }
    }
    void update(float elapsed)
    {
        for (int i = 0; i < bullets.size(); i++) {
            bullets[i].update(elapsed);
            if (bullets[i].y > 2.0f)
            {
                bullets[i].dead = true;
            }
            if (bullets.back().dead)
            {
                bullets.pop_back();
            }
        }
    }
    void draw(ShaderProgram &program)
    {
        for (int i = 0; i < bullets.size(); i++) {
            bullets[i].Draw(program);
        }
    }
};

struct EntArmy
{
    EntArmy() {};
    std::vector<Entity> ents;
    float width = 1.5f;
    float height = .75f;
    float x = 0.0f;
    float y = 0.6f;
    int xdir = 1.0f;
    float speedup;
    float alive;
    void increasespeed()
    {
        for (int i = 0; i < ents.size(); i++)
        {
            ents[i].velocity+= .1;
            ents[i].y -= .1;
        }
    }
    void populate(GLuint texid)
    {
        int counterx = 0;
        int countery = 0;
        for (float ax = x + -((x + width) / 2); ax < x + ((x + width) / 2); ax += ((x + width) / 5))
        {
            for (float ay = y + (-((height) / 2)); ay < y + (height / 2); ay += (height / 5))
            
            {
                Entity myEntity(ax, ay, .1, .1, texid,.3,0);
                myEntity.sprite = SheetSprite(texid, 425.0f / 1024.0f, 468.0f / 1024.0f,93.0f / 1024.0f, 84.0f / 1024.0f, 1);
                ents.push_back(myEntity);
                countery += 1;
            }
            counterx += 1;
            countery = 0;
        }
    }
    void col(Entity* ent)
    {
        for (int i = 0; i < ents.size(); i++) {
            bool col = ents[i].collision(ent);
            if (!ents[i].dead)
            {
                if (col && ent->id == 1)
                {
                    if (ents[i].x < 0)
                    {
                        if (xdir == -1)
                        {
                            increasespeed();
                        }
                        xdir = 1;
                    }
                    else if (ents[i].x > 0)
                    {
                        if (xdir == 1)
                        {
                            increasespeed();
                        }
                        xdir = -1;
                    }
                }
                else if (col && ent->id == 4)
                {
                    ent->dead = true;
                    ents[i].dead = true;
                }
                else if (col && ent->id == 2)
                {
                    ent->dead = true;
                }
            }
        }
        
    }
    void upd(float elapsed) {
        //glClear(GL_COLOR_BUFFER_BIT);
        //speedup = .5 * (1.0f - ((float)(alive-2) / 25));
        for (int i = 0; i < ents.size(); i++) {
            ents[i].update(elapsed*xdir);
        }
    }
    void Render(ShaderProgram &program) {
        glClear(GL_COLOR_BUFFER_BIT);
        for (int i = 0; i < ents.size(); i++) {
            ents[i].Draw(program);
        }
    }
    void bulletcheck(bullethell* bullet, float elapsed)
    {
        for (int i = 0; i < bullet->bullets.size(); i++)
        {
            col(&bullet->bullets[i]);
        }
    }
};

int main(int argc, char *argv[])
{
    float lastFrameTicks = 0.0f;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl",
                 RESOURCE_FOLDER"fragment_textured.glsl");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    GLuint d = LoadTexture(RESOURCE_FOLDER"font1.png");
    GLuint e = LoadTexture(RESOURCE_FOLDER"sheet.png");
    EntArmy r;
    Entity left = Entity(-1.877f, 0, .2f, 3.0f, 0, 0, 1);
    Entity right = Entity(1.877f, 0, .2f, 3.0f, 0, 0, 1);
    Entity player = Entity(0, -.7f, .2, .2, e, .0001, 2);
    bullethell bulletpool(e);
    player.sprite = SheetSprite(e, 211.0f / 1024.0f, 941.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 1);
    r.populate(e);
    //SheetSprite(d,)
    text space;
    text lose;
    lose.textval = "THEGAMEISOVER";
    lose.lettersheet = d;
    lose.place_text_ongame();
    space.textval = "SPACE INVADERS";
    space.lettersheet = d;
    space.place_text_ongame();
    glUseProgram(program.programID);
    program.SetProjectionMatrix(projectionMatrix);
    SDL_Event event;
    bool done = false;
    bool start = true;
    bool gameover = false;
    while (!done) {
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        if (keys[SDL_SCANCODE_SPACE])
        {
            if (start)
            {
                start = false;
            }
            else if (gameover)
            {
                done = true;
            }
            else
            {
                bulletpool.shoot(player.x);
            }
            //entleft.movepaddle(elapsed, 1);
            //entright.movepaddle(elapsed, 1);
        }
        if (keys[SDL_SCANCODE_LEFT])
        {
            player.x -= .0001;
        }
        if (keys[SDL_SCANCODE_RIGHT])
        {
            player.x += .0001;
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(.5f, .3f, .6f, 1.0f);
        //letterE.DrawSpriteSheetSprite(program, 69, 16, 16);
        if (start)
        {
            space.draw(program);
        }
        else if (gameover)
        {
            lose.draw(program);
        }
        else
        {
            r.col(&left);
            r.col(&right);
            r.col(&player);
            r.upd(elapsed);
            bulletpool.update(elapsed);
            r.Render(program);
            r.bulletcheck(&bulletpool, elapsed);
            player.update(elapsed);
            if (player.dead)
            {
                gameover = true;
            }
            bulletpool.draw(program);
            player.Draw(program);
        }
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}

