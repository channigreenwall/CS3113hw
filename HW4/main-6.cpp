#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glm/ext.hpp"
#include "ShaderProgram.h"
#include <vector>
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float sizeoftiles = .1f;
float frick = 1.0f;
float grav = -1.0f;
#define pie 3.14159265
SDL_Window* displayWindow;
float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}
GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		//std::assert(false);
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

class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID1, float textindex, int countx, float county, float size1)
	{
		textureID = textureID1;
		texindex = textindex;
		Countx = countx;
		County = county;
		size = size1;
	};
	//void Draw(ShaderProgram &program,float* texCoords, float* vertCoords);
	float size;
	unsigned int textureID;
	float texindex;
	int Countx;
	float County;
	void DrawSpriteSheetSprite(ShaderProgram &program ) {

		float u = (float)(((int)texindex) % Countx) / (float)Countx;
		float v = (float)(((int)texindex) / Countx) / (float)County;
		float spriteWidth = 1.0 / (float)Countx;
		float spriteHeight = 1.0 / (float)County;
		//glm::translate(modelMatrix, glm::vec3(2.0f,1.0f,1.0f));
		float texeCoords[] = {
			/*u, v + spriteHeight,
			u + spriteWidth, v + spriteHeight,
			u + spriteWidth, v,
			u, v + spriteHeight,
			u + spriteWidth, v,
			u, v*/
			u, v + spriteHeight,
			u + spriteWidth, v,
			u, v,
			u + spriteWidth, v,
			u, v + spriteHeight,
			u + spriteWidth, v + spriteHeight
		};
		float val[12] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,/*half*/ 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
		Draw(program, val, texeCoords);
	}
	void Draw(ShaderProgram program, float* vertCoords,float* texCoords) {
		//std::cout << "Model" << std::endl;
		//std::cout << vertices[0] << std::endl;
		glBindTexture(GL_TEXTURE_2D, textureID);
		//std::cout << "Verts" << std::endl;
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertCoords);
		glEnableVertexAttribArray(program.positionAttribute);
		//std::cout << "texs" << std::endl;
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	/*void DrawSpriteUnorder(ShaderProgram &program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texeCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size ,
			0.5f * size * aspect, -0.5f * size };
		// draw our arrays
		Draw(program, vertices, texeCoords);
	}*/
};

struct Entity { //store all info for every object
				//float flipx = 1; //when colliding with paddle flips x axis val
	Entity() {};
	bool dead = false;
	bool floortouch = false;
	int xdir = 1;
	float x;
	float y;
	float maxy = .88f; //roof of game
	float maxx = 1.7f; //walls of game
					   //float rotation;
	GLuint textureID; //pointer to spritesheet or lettersheet
	float width; //width/heights of objects themselves
	float height;
	glm::vec3 velocity; //speed an entity is going
					//float direction_x = 1;
	float direction_y = 1; //flip for y axis
						   //float angle=45; //angle ball is going
	float id; //tells program weather alien(0), bullet(1) or ship(2)
	SheetSprite sprite;
	glm::mat4 viewMatrix = glm::mat4(1.0f); //where vertices are stored
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	Entity(float x1, float y1, float width1, float height1, GLuint text1, glm::vec3 vel1, float identification,int textin, int countx, int county) {
		x = x1;
		y = y1;
		width = width1;
		height = height1;
		textureID = text1;
		velocity = vel1;
		id = identification;
		sprite = SheetSprite(text1,textin,countx,county,1.0f);
	}
	//individual letter coordinates on sheet (like A's for example)
	bool collision(Entity* e) {
		//what is causing collision
		bool rightpadde = e->x >= 0; //check to see which we are colliding with, if x>0 not left
		bool leftpaddle = e->x <= 0;//no way right paddle					//collision of any two boxes
		float paddleroof = e->y + (e->height / 2.0f); //top middle
		float paddlefloor = e->y - (e->height / 2.0f);//bottom moddle
		float paddleleft = e->x - (e->width / 2.0f);
		float paddleright = e->x + (e->width / 2);
		float ballroof = y + (height / 2);//top point of ball
		float ballfloor = y - (height / 2);
		float ballright = x + (width / 2);
		float ballleft = x - (width / 2);
		bool ballxpaddle = ((ballright >= paddleleft && ballleft <= paddleright) || (ballright >= paddleleft &&
			ballleft < paddleleft));
		bool inrange = (ballfloor <= paddleroof && ballxpaddle && ballroof >= paddlefloor);
		return inrange;
	}
	void Draw(ShaderProgram &program) {
		//std::cout << "Model" << std::endl;
		if (!dead)
		{
			//modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::scale(modelMatrix, glm::vec3(width, height, 0.0f));
			program.SetModelMatrix(modelMatrix);
			//program.SetViewMatrix(viewMatrix);
			sprite.DrawSpriteSheetSprite(program);
		}
	}

	void update(float elapsed) { //keeps game and computer in tandem
								 //double xdir = cos(angle*pie/180);
		//y += elapsed * velocity * ydir * direction_y *stopper;
		if (!dead)
		{
			if (id == 1 && floortouch)
			{
				velocity[1] = .9f;
				floortouch = false;
				velocity[0] = .01f;
			}
			if (velocity[1] != -.1f)
			{
				velocity[1] -= .001f;
			}
			x += elapsed * velocity[0];
			velocity[0] = lerp(velocity[0], 0.0f, elapsed * frick);
			y = y + elapsed * velocity[1];
			//floa		}
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
		}
	}
};

struct text {
	float text_x = 0;
	float text_y = 0;
	float text_width = 2.5;
	float text_height = 1.5;
	float fontsize = .3;
	std::string textval;
	SheetSprite* letter_and_spriteholder[13];
	GLuint lettersheet;
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	void place_text_ongame() {
		int counterx = 0; //counter to loop through array as we add new vals
		for (float ax = text_x + -(text_width / 2); ax < text_x + ((text_width) / 2); ax += ((text_x + text_width) / 13)) {
				letter_and_spriteholder[counterx] = new SheetSprite(lettersheet, (int)(textval[counterx]),16,16,0); //creating the
					counterx++;
			}
	}
	void draw(ShaderProgram program) {
		int counterx = 0;
		program.SetViewMatrix(viewMatrix);
		for (float ax = text_x + -(text_width / 2); ax < text_x + ((text_width) / 2); ax += ((text_x + text_width) /
			13)) {
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(ax, 0.0f, 1.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(fontsize, fontsize, 1.0f));
			program.SetModelMatrix(modelMatrix);
			letter_and_spriteholder[counterx]->DrawSpriteSheetSprite(program);
			counterx++;
		}
	}
};
struct LevelData {
	int mapWidth = -1;
	int mapHeight = -1;
	unsigned int **actlevelData;
	Entity Player;
	std::vector<Entity*> entities;
	std::vector<SheetSprite> blocks;
	unsigned int sheetlen = 0;
	std::vector<float> xvalse;
	std::vector<float> yvalse;
	glm::mat4 burnMatrix = glm::mat4(1.0f);
	GLuint sheet;

	bool readHeader(std::ifstream &stream) {
		std::string line;
		mapWidth = -1;
		mapHeight = -1;
		while (getline(stream, line)) {
			if (line == "") { break; }
			std::istringstream sStream(line);
			std::string key, value;
			std::getline(sStream, key, '=');
			std::getline(sStream, value);
			if (key == "width") {
				mapWidth = atoi(value.c_str());
			}
			else if (key == "height") {
				mapHeight = atoi(value.c_str());
			}
		}
		if (mapWidth == -1 || mapHeight == -1) {
			return false;
		}
		else { // allocate our map data
			actlevelData = new unsigned int*[mapHeight];
			for (int i = 0; i < mapHeight; ++i) {
				actlevelData[i] = new unsigned int[mapWidth];
			}
			return true;
		}
	}
	
	void genblocks()
	{
		for (int y = 0; y < mapHeight; y++) {
			for (int x = 0; x < mapWidth; x++) {
				float val = actlevelData[y][x];
				if (val != 0)
				{
					blocks.push_back(SheetSprite(sheet, val, 16, 8, .1f));
					xvalse.push_back((float)sizeoftiles*x);
					yvalse.push_back((float)sizeoftiles*-y);
					sheetlen += 1;
				}
			}
		}
	}
	bool readLayerData(std::ifstream &stream) {
		std::string line;
		while (getline(stream, line)) {
			if (line == "") { break; }
			std::istringstream sStream(line);
			std::string key, value;
			std::getline(sStream, key, '=');
			std::getline(sStream, value);
			if (key == "data") {
				for (int y = 0; y < mapHeight; y++) {
					std::getline(stream, line);
					std::istringstream lineStream(line);
					std::string tile;
					for (int x = 0; x < mapWidth; x++) {
						std::getline(lineStream, tile, ',');
						unsigned int val = atoi(tile.c_str());
						if (val > 0) {
							// be careful, the tiles in this format are indexed from 1 not 0
							actlevelData[y][x] = val - 1;
						}
						else {
							actlevelData[y][x] = 0;
						}
					}
				}
			}
		}
		genblocks();
		return true;
	}
	void genEnt(int id, float x, float y)
	{
		int p = 0;
		if (id == 1) {
			Entity* newObj = new Entity(x, y*.8f, .1f, .1f, sheet, glm::vec3(0.0f, 0.0f, 0.0f), 1, 59, 16, 8);
			entities.push_back(newObj);
		}
		else if (id == 0) {
			Player = (Entity(x*.8f, y*.8f, .1f, .1f, sheet, glm::vec3(0.0f,0.0f,0.0f), id, 80, 16, 8));
		}
	}
	bool readEntityData(std::ifstream &stream) {
		std::string line;
		std::string type;
		while (std::getline(stream, line)) {
			if (line == "") { break; }
			std::istringstream sStream(line);
			std::string key, value;
			std::getline(sStream, key, '=');
			std::getline(sStream, value);
			if (key == "type") {
				type = value;
			}
			else if (key == "location") {
				std::istringstream lineStream(value);
				std::string xPosition, yPosition;
				std::getline(lineStream, xPosition, ',');
				std::getline(lineStream, yPosition, ',');
				float placeX = atoi(xPosition.c_str())* sizeoftiles;
				float placeY = atoi(yPosition.c_str())*-sizeoftiles;
				if (type == "Enemy")
				{
					genEnt(0, placeX, placeY);
				}
				else
				{
					genEnt(1, placeX, placeY);
				}
			}
		}
		return true;
	}
	void loadThings(std::string &name) {
		sheet = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
		std::ifstream infile(name);
		std::string line;
		while (std::getline(infile, line)) {
			if (line == "[header]") {
				if (!readHeader(infile)) {
					return;
				}
			}
			else if (line == "[layer]") {
				readLayerData(infile);
			}
			else if (line == "[Object]") {
				readEntityData(infile);
			}
		}
	}
	void Draw(ShaderProgram &program)
	{
		for (Entity* i : entities)
		{
			i->Draw(program);
		}
  		Player.Draw(program);
		for (unsigned int d = 0; d < sheetlen; d++)
		{
			burnMatrix = glm::mat4(1.0f);

			burnMatrix = glm::translate(burnMatrix, glm::vec3(xvalse[d], yvalse[d], 1));
			burnMatrix = glm::scale(burnMatrix, glm::vec3(.1f, .1f, 0.0f));

			//burnMatrix = glm::scale
			program.SetModelMatrix(burnMatrix);
			blocks[d].DrawSpriteSheetSprite(program);
		}
	}
	void collx(Entity* obj)
	{
		int xvals = (int)(obj->x / sizeoftiles);
		int yvals = (int)(-obj->y / sizeoftiles);
		for (int y = 0; y < mapHeight; y++)
		{
			for (int x = 0; x < mapWidth; x++)
			{
				int val = actlevelData[y][x];
				if (val != 0 && val != 14 && val != 30) {
					if ((obj->x - fabs(obj->width)) <= (sizeoftiles * x)&& xvals >= x && yvals == y)
					{
						obj->velocity[0] = 0;
						obj->x += fabs(((obj->x - (fabs(obj->width)))) - ((sizeoftiles * x))) + 0.0000001f;
					}
					if ((obj->x + fabs(obj->width)) >= (sizeoftiles * x) && xvals <= x && yvals == y)
					{
						obj->velocity[0] = 0;
						obj->x -= fabs((obj->x + (fabs(obj->width)))) - ((sizeoftiles * x)) - 0.0000001f;
					}
				}
			}
		}
		/*int xvals = (int)(obj->x / sizeoftiles);
		int yvals = (int)(-obj->y / sizeoftiles);
		//worldToTileCoordinates(obj->x, obj->y, xvals, yvals);
		bool leftcol = false;
		bool rightcol = false;
		for (int i = 0; i < sheetlen; i++) {
			if ((obj->x - fabs(obj->width / 2)) <= (sizeoftiles * (xvalse[i])) + sizeoftiles && xvals >= xvalse[i] && yvals == yvalse[i])
			{
				obj->velocity[0] = 0;
				obj->x += fabs(((obj->x - (fabs(obj->width) / 2))) - ((sizeoftiles * xvalse[i]) + sizeoftiles)) + 0.0000000001f;
			}
			if ((obj->x + fabs(obj->width / 2)) >= (sizeoftiles * xvalse[i]) && xvals <= xvalse[i] && yvals == yvalse[i])//(obj->y - (obj->height / 2)) == (-sizeoftiles * y - sizeoftiles))
			{
				obj->velocity[0] = 0;
				obj->x -= fabs((obj->x + (fabs(obj->width) / 2))) - ((sizeoftiles * xvalse[i])) + 0.0000000001f;
			}
		}*/
	}

	void colly(Entity* obj)
	{
		int xvals = (int)(obj->x / sizeoftiles);
		int yvals = (int)(-obj->y / sizeoftiles);
		bool downcol = false;
		bool upcol = false;
		for (int y = 0; y < mapHeight; y++)
		{
			for (int x = 0; x < mapWidth; x++)
			{
				int val = actlevelData[y][x];
				if (val != 0 && val!= 14 && val != 30) {
					if (((obj->y ) - (obj->height)) <= (-sizeoftiles * y) && xvals == x && obj->y + (obj->height / 2) >(-sizeoftiles * y))
					{
						obj->velocity[1] = 0;
						obj->y += fabs((obj->y - (fabs(obj->width))) - (-sizeoftiles * y)) + .0000001f;
						obj->floortouch = true;
					}

				    if (((obj->y) + (obj->height)) >= (-sizeoftiles * y) && xvals == x && obj->y - (obj->height / 2) < (-sizeoftiles * y) - sizeoftiles)
					{
						obj->velocity[1] = 0;
						obj->y -= fabs((obj->y + (fabs(obj->width))) - ((-sizeoftiles * y))) - 0.0000001f;
					}
				}
			}
		}
	}
	void collisions(float elapsed)
	{
		for (Entity* i : entities)
		{
			collx(i);
			colly(i);
		}
		collx(&Player);
		colly(&Player);
		if (Player.collision(entities[0]))
		{
			Player.dead = true;
		}
	}
	void update(float elapsed)
	{
		for (Entity* i : entities)
		{
			i->update(elapsed);
		}
		Player.update(elapsed);
	}
};

/*
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
};*/

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
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	GLuint d = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint e = LoadTexture(RESOURCE_FOLDER"sheet.png");
	//SheetSprite(d,)
	text space;
	text lose;
	LevelData World;
	std::string val = "slimeTime.txt";
	World.loadThings(val);
	lose.textval = "THEGAMEISOVER";
	lose.lettersheet = d;
	lose.place_text_ongame();
	space.textval = "PLATS INVADERS";
	space.lettersheet = d;
	space.place_text_ongame();
	glUseProgram(program.programID);
	program.SetProjectionMatrix(projectionMatrix);
	SDL_Event event;
	bool done = false;
	bool start = true;
	bool gameover = false;
	float accumulator = 0.0f;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
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
				if (World.Player.floortouch)
				{
					World.Player.velocity[1] = .9f;
					World.Player.floortouch = false;
				}
			}
			//entleft.movepaddle(elapsed, 1);
			//entright.movepaddle(elapsed, 1);
		}
		if (keys[SDL_SCANCODE_LEFT])
		{
			World.Player.velocity[0] = .2f;
		}
		if (keys[SDL_SCANCODE_RIGHT])
		{
			World.Player.velocity[0] = -.2f;
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
			while (elapsed >= FIXED_TIMESTEP) {

				elapsed -= FIXED_TIMESTEP;
			}
			World.update(elapsed);
			World.collisions(elapsed);
			if (World.Player.dead)
			{
				gameover = true;
			}
			World.Draw(program);
			viewMatrix = glm::mat4(1.0f);
			//viewMatrix = glm::scale(viewMatrix, glm::vec3(1 / sizeoftiles, 1 / sizeoftiles, 1));
			viewMatrix = glm::translate(viewMatrix, glm::vec3(-World.Player.x, -World.Player.y, (0.0f)));
			program.SetViewMatrix(viewMatrix);
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
