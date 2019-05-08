//L1 catch 1 boy
//steady timer in corner 10 secs
//L2 catch 2 boys
//steady timer in corner 15 secs
//L3 catch 3 boys
//steady timer in corner 20 secs
//purposely collision with clouds

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "FlareMap.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define TILE_SIZE 0.13f
#include <SDL_mixer.h>

enum GameMode{ MAIN_MENU, GAME_LEVEL1, GAME_LEVEL2, GAME_LEVEL3, GAME_OVER};
glm::mat4 viewMatrix = glm::mat4(1.0f); //4 by 4 matrix

GameMode mode=MAIN_MENU;
ShaderProgram program;
SDL_Window* displayWindow;
Mix_Music *music;
Mix_Chunk *sound;

//images mapped to index pass in index of image you want to draw from
void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX,
                           int spriteCountY,GLuint texture) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    float texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f*TILE_SIZE, -0.5f*TILE_SIZE, 0.5f*TILE_SIZE, 0.5f*TILE_SIZE, -0.5f*TILE_SIZE, 0.5f*TILE_SIZE, 0.5f*TILE_SIZE, 0.5f*TILE_SIZE,  -0.5f*TILE_SIZE,
        -0.5f*TILE_SIZE, 0.5f*TILE_SIZE, -0.5f*TILE_SIZE};
    glBindTexture(GL_TEXTURE_2D, texture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6); //6 vertices per thing
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

class Entity{
public:
    Entity(){
        velocity.x=1.0f;
        velocity.y=-1.0f; //gravity
        
    }
    void Draw(ShaderProgram &p){
        glm::mat4 modelMatrix = glm::mat4(1.0f); //reset to 0,0 then translate to actual position
        //before drawing player define modelmatrix ie player position
        modelMatrix = glm::translate(modelMatrix, glm::vec3(x,y,0.0f)); //translate by player position so it gives correct position each frame and isn't off
        p.SetModelMatrix(modelMatrix);
        DrawSpriteSheetSprite(p,index, spritesheetW, spritesheetH, textureID); //ex if index =3, W=4&H=4 , pick girl 3 on betty sheet that is 4 by 4
    }
    float x;
    float y;
    float rotation;
    int index;
    int spritesheetW;
    int spritesheetH;
    GLuint textureID;
    float width;
    float height;
    glm::vec3 velocity;//3 rows 1 column
    glm::vec3 acceleration;//3 rows 1 column
    bool collidedbottom=false;
};
float frictionx=0.5f;
float frictiony=0.5f;
Entity betty;
Entity george;
FlareMap map;
GLuint spritesheettexture;
GLuint bettytexture;
GLuint georgetexture;
GLuint fonttexture;

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

void DrawTileMap(ShaderProgram& program) {
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int y=0; y < map.mapHeight; y++) {
        for(int x=0; x < map.mapWidth; x++) {
            if (map.mapData[y][x] != 0){
                float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
                vertexData.insert(vertexData.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+spriteHeight,
                    
                    u, v,
                    u+spriteWidth, v+spriteHeight,
                    u+spriteWidth, v
                });
            }
            
        }
    }
    glBindTexture(GL_TEXTURE_2D, spritesheettexture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2); //6 vertices per thing
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

}

void renderlevel1(ShaderProgram& program){
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    DrawTileMap(program);
    betty.Draw(program);
    george.Draw(program);
    //follow betty with camera
    viewMatrix = glm::mat4(1.0f);
    //was at mid screen 0,0 so we change view matrix so we see it differently ie translate ir
    viewMatrix = glm::translate(viewMatrix, glm::vec3(std::min(-1.77f, -betty.x),std::min(1.0f, -betty.y),0.0f)); //vector of 3
    program.SetViewMatrix(viewMatrix);
}

void renderlevel2(ShaderProgram& program){
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    DrawTileMap(program);
    betty.Draw(program);
    george.Draw(program);
    //follow betty with camera
    viewMatrix = glm::mat4(1.0f);
    //was at mid screen 0,0 so we change view matrix so we see it differently ie translate ir
    viewMatrix = glm::translate(viewMatrix, glm::vec3(std::min(-1.77f, -betty.x),std::min(1.0f, -betty.y),0.0f)); //vector of 3
    program.SetViewMatrix(viewMatrix);
}

void renderlevel3(ShaderProgram& program){
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    DrawTileMap(program);
    betty.Draw(program);
    george.Draw(program);
    //follow betty with camera
    viewMatrix = glm::mat4(1.0f);
    //was at mid screen 0,0 so we change view matrix so we see it differently ie translate ir
    viewMatrix = glm::translate(viewMatrix, glm::vec3(std::min(-1.77f, -betty.x),std::min(1.0f, -betty.y),0.0f)); //vector of 3
    program.SetViewMatrix(viewMatrix);
}

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
    float character_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x + character_size, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x, texture_y + character_size,
        }); }
    glBindTexture(GL_TEXTURE_2D, fonttexture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2); //6 vertices per thing
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    // draw this data (use the .data() method of std::vector to get pointer to data)
    // draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices
}

void rendermainmenu(ShaderProgram& program){
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.6,0,0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(program, fonttexture, "Girls Run the World", 0.1f, 0.0f);
}

void rendergameover(ShaderProgram& program){
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.6,0,0.0f));
    program.SetModelMatrix(modelMatrix);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix); //reset cam to 0,0 because u oved player so u need it to move with player so you can see text
    DrawText(program, fonttexture, "Couldn't Catch Him", 0.17f, 0.0f);
}

void render(ShaderProgram& program){
    switch(mode){
        case MAIN_MENU:
            rendermainmenu(program);
            break;
        case GAME_LEVEL1:
            renderlevel1(program);
            break;
        case GAME_LEVEL2:
            renderlevel2(program);
            break;
        case GAME_LEVEL3:
            renderlevel3(program);
            break;
        case GAME_OVER:
            rendergameover(program);
            break;
    }
}

//makes mid val value between the 2
float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

//converts world coords into tiled coords
void worldToTileCoordinates(float worldX, float worldY, int& gridX, int& gridY) {
    gridX = (int)(worldX / TILE_SIZE);
    gridY = (int)(worldY / -TILE_SIZE);
}

bool collision(Entity& a, Entity& b){
    float p1=fabs(a.x-b.x)-(a.width+b.width/2.0f);
    float p2=fabs(a.y-b.y)-(a.height+b.height/2.0f);
    if(p1<0 && p2<0){
        return true; //because collision
    }
    return false;
}

float animationTime =0.0f; //how much time has passed

float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
    float retVal = dstMin + ((value - srcMin)/(srcMax-srcMin) * (dstMax-dstMin));
    if(retVal < dstMin) {
        retVal = dstMin;
    }
    if(retVal > dstMax) {
        retVal = dstMax;
    }
    return retVal;
}

float levelTime = 20.0f;
int boyscaught = 0;
void updatelevel1(float elapsed){
    
    
    
    animationTime=animationTime+elapsed; //num of seconds that have passed in real time
    float animationValue = mapValue(animationTime, 0.2f, 10.5f, 0.0f, 1.0f); //from second 2 to sec 10.5 move right
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    float xPos = lerp(0.0f, 3.0f, animationValue);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(xPos, -0.5f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(program, fonttexture, "LEVEL ONE", 0.25f, 0.0f);
    
    //draw timer
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(betty.x, betty.y + 0.1f, 0.0f));
    program.SetModelMatrix(modelMatrix);
    float remainingTime = levelTime - animationTime;
    DrawText(program, fonttexture, std::to_string((int)remainingTime), 0.1f, 0.0f);
    if (remainingTime<=0){
        if(boyscaught>0){
            mode=GAME_LEVEL2;
        }
        else{
            mode=GAME_OVER;
        }
    }

    betty.collidedbottom=false;
    betty.acceleration.x=0.0f;
    betty.acceleration.y=-1.0f;//speed
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_LEFT]) {
        // go left!
        betty.acceleration.x=-1.0f;
    } else if(keys[SDL_SCANCODE_RIGHT]) {
        // go right!
        betty.acceleration.x=1.0f;
    }
    betty.velocity.x = lerp(betty.velocity.x, 0.0f, elapsed * frictionx);
    betty.velocity.y = lerp(betty.velocity.y, 0.0f, elapsed * frictiony);
    betty.velocity.x += betty.acceleration.x * elapsed;
    betty.velocity.y += betty.acceleration.y * elapsed;
    betty.x += betty.velocity.x * elapsed;
    //left collision
    int gridx;
    int gridy;
    worldToTileCoordinates(betty.x-betty.width/2, betty.y, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs(((TILE_SIZE*gridx)+TILE_SIZE)-(betty.x-betty.width/2));
        betty.x +=penetration; //push her up by that much right because you are going left
    }
    //right collision
    worldToTileCoordinates(betty.x+betty.width/2, betty.y, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs((TILE_SIZE*gridx)-(betty.x+betty.width/2));
        betty.x -=penetration; //move right so push left
    }
    betty.y += betty.velocity.y * elapsed; //move betty
    //bottom collision to see if hit floor
    worldToTileCoordinates(betty.x, betty.y-betty.height/2, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs((-TILE_SIZE*gridy)-(betty.y-betty.height/2));
        betty.y +=penetration; //push her up by that much
        betty.collidedbottom=true;
    }
    //top collision to see if hit cieling
    worldToTileCoordinates(betty.x, betty.y+betty.height/2, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs(((-TILE_SIZE*gridy)-TILE_SIZE)-(betty.y-betty.height/2));
        //penetration how much she went up
        betty.y -=penetration; //push her down by that much
    }
    
    //george movement
    george.x+=george.velocity.x*elapsed;
    //left collision
    worldToTileCoordinates(george.x-george.width/2, george.y, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs(((TILE_SIZE*gridx)+TILE_SIZE)-(george.x-george.width/2));
        george.x +=penetration; //push her up by that much right because you are going left
        george.velocity.x *= -1; //if he collides push him other way
    }
    //right collision
    worldToTileCoordinates(george.x+george.width/2, george.y, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs((TILE_SIZE*gridx)-(george.x+george.width/2));
        george.x -=penetration; //move right so push left
        george.velocity.x *= -1; //if he collides push him other way
    }
    george.y += george.velocity.y * elapsed; //move betty
    //bottom collision to see if hit floor
    worldToTileCoordinates(george.x, george.y-george.height/2, gridx, gridy);
    if(gridy>=0 && gridx>=0 && map.mapData[gridy][gridx]!=0){
        //so you are standing on something
        float penetration = fabs((-TILE_SIZE*gridy)-(george.y-george.height/2));
        george.y +=penetration; //push him up by that much
        george.collidedbottom=true;
    }
    
    
    if(collision(betty, george)){
        Mix_PlayChannel(-1, sound, 0);
        george.x=50000.0f;//if rthey collide mov george offscreen
        boyscaught+=1;
        mode=GAME_LEVEL2;
    }
}
void updatelevel2(float elapsed){}

void updatelevel3(float elapsed){}

//use elapsed time because different comps diff speeds so makes game run same on all comps
void update(float elapsed){
    switch(mode){ //check mode
        case MAIN_MENU:
            break;
        case GAME_LEVEL1:
            updatelevel1(elapsed);
            break;
        case GAME_LEVEL2:
            updatelevel2(elapsed);
            break;
        case GAME_LEVEL3:
            updatelevel3(elapsed);
            break;
    }
}

int main(int argc, char *argv[])
{
    float lastFrameTicks = 0.0f;
    float accumulator = 0.0f;
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    glViewport(0, 0, 640, 360);
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl",RESOURCE_FOLDER"fragment_textured.glsl");
    spritesheettexture=LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    bettytexture=LoadTexture(RESOURCE_FOLDER"betty_0.png");
    georgetexture=LoadTexture(RESOURCE_FOLDER"george_0.png");
    fonttexture=LoadTexture(RESOURCE_FOLDER"font1.png");
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    map.Load(RESOURCE_FOLDER"level1.txt");
    for(int i=0; i<map.entities.size();i++){
        if(map.entities[i].type=="Player"){
            //convert tile coords to game coords
            float gameX = map.entities[i].x*TILE_SIZE;
            float gameY = map.entities[i].y*-TILE_SIZE;
            //now these are my players coordinates
            betty.x=gameX;
            betty.y=gameY;
            betty.textureID=bettytexture;
            betty.index=3;
            betty.spritesheetH=4;
            betty.spritesheetW=4;
            betty.width=TILE_SIZE;
            betty.height=TILE_SIZE;
        }
        else if(map.entities[i].type=="Enemy"){
            //convert tile coords to game coords
            float gameX = map.entities[i].x*TILE_SIZE;
            float gameY = map.entities[i].y*-TILE_SIZE;
            //now these are my players coordinates
            george.x=gameX;
            george.y=gameY;
            george.textureID=georgetexture;
            george.index=7;
            george.spritesheetH=4;
            george.spritesheetW=4;
            george.width=TILE_SIZE;
            george.height=TILE_SIZE;
            george.velocity.x = 0.5f;
            george.velocity.y = -1.0f;
        }
        
        
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    music=Mix_LoadMUS(RESOURCE_FOLDER"run.mp3");
    Mix_PlayMusic(music, -1);
    sound= Mix_LoadWAV(RESOURCE_FOLDER"catch.wav");
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            if(event.type==SDL_KEYDOWN){
                if (mode == MAIN_MENU) {
                    if(event.key.keysym.scancode == SDL_SCANCODE_SPACE){//space was pressed
                        mode=GAME_LEVEL1;
                    }
                }
                else if (mode == GAME_LEVEL1) {
                    if(betty.collidedbottom && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
                        betty.velocity.y+=3.0f;
                    }
                }
                
                
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);

        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue; }
        while(elapsed >= FIXED_TIMESTEP) {
            update(FIXED_TIMESTEP); //do diff things if on main menu or game level
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        render(program);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
