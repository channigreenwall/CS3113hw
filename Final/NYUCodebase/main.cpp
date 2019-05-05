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

SDL_Window* displayWindow;
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

void render(ShaderProgram& program){
    glClear(GL_COLOR_BUFFER_BIT);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    DrawTileMap(program);
    betty.Draw(program);
    george.Draw(program);
    //follow betty with camera
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    //was at mid screen 0,0 so we change view matrix so we see it differently ie translate ir
    viewMatrix = glm::translate(viewMatrix, glm::vec3(std::min(-1.77f, -betty.x),std::min(1.0f, -betty.y),0.0f)); //vector of 3
    program.SetViewMatrix(viewMatrix);
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
//use elapsed time because different comps diff speeds so makes game run same on all comps
void update(float elapsed){
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
    if(collision(betty, george)){
        george.x=50000.0f;//if rthey collide mov george offscreen
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
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl",RESOURCE_FOLDER"fragment_textured.glsl");
    spritesheettexture=LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    bettytexture=LoadTexture(RESOURCE_FOLDER"betty_0.png");
    georgetexture=LoadTexture(RESOURCE_FOLDER"george_0.png");
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    
    glm::mat4 viewMatrix = glm::mat4(1.0f); //4 by 4 matrix

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
        }
        
        
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            if(event.type==SDL_KEYDOWN){
                if(betty.collidedbottom && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
                    betty.velocity.y+=3.0f;
                }
            }
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue; }
        while(elapsed >= FIXED_TIMESTEP) {
            update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        render(program);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
