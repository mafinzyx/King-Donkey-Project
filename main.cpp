#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH		  1280
#define SCREEN_HEIGHT		  1000


#define MAX_PLATFORMS		  50
#define SPAWN_PLATFORM		  9

#define MAX_LADDERS			  50

#define MAX_POINTS            50
#define MAX_LIVES             50

#define SPAWN__PLATFORM_Y	  980
#define SPAWN_PLATFORM_X      50

#define PLATFORM_LENGTH       150

#define LEFT_BORDER_PLATFORM  21
#define RIGHT_BORDER_PLATFORM 1259

#define START_X               650
#define START_Y               840

#define GRAVITY				  400
#define JUMP_HEIGHT			  250.0

#define TOP_OF_LADDER         65

#define DELETE_FROM_BOARD     -50

#define SECONDS_BETWEEN_REFRESH 0.5
#define REFRESH_RATE 1/SECONDS_BETWEEN_REFRESH

#define MAX_NICKNAME_LENGTH 20
char inputText[MAX_NICKNAME_LENGTH] = "";


struct Vector2 {
	int x;
	int y;
};

struct Game {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* playerModel;
	SDL_Texture* scrtex;
	SDL_Texture* set;

	int timer1, timer2, quit = 0, frames = 0, rc;
	int black, green, red, blue, score;
	double delta, worldTime = 0, fpsTimer = 0, fps = 0, distance, etiSpeed;
	char text[128];

	int currentLevel = 0;
	int currentPage = 1;

	bool Menu = true, Records = false;
	bool Level1 = false, Level2 = false, Level3 = false;
	bool EndLevel = false, EndGame = false;

	bool IsUnevenPlatform = false;

	bool canWrite = false;
	bool isEnterNickname = false;
	char nickname[256];

	bool goToNextPage = false, goToPreviousPage = false;
};

struct PointsText {
	Vector2 V2;
	int count;
};

struct Points {
	Vector2 V2;
	PointsText pointsText;

	bool isCollected = false;
	bool first = false, second = false, third = false;

	bool hidePoint1 = false, hidePoint2 = false, hidePoint3 = false;

	SDL_Surface* image;
	const char* imageFilePoints;
};

struct PlatformData {
	Vector2 V2;

	int width;
	int height;
};

struct Player {
	Vector2 V2;
	int width, height;
	int speed;
	double verticalVelocity = 0.0;
	double jumpStartY, jumpVelocity;
	bool isJumping;

	bool isMovingLeft, isMovingRight;

	bool leftBorder, rightBorder;

	bool ItsLadderHereUp, ItsLadderHereDown;
	bool CanMoveUp, CanMoveDown;

	bool playerColliding = false;
};

typedef struct {
	Vector2 V2;
	SDL_Surface* image;
	int lives_count = 3;

	const char* imageFileLives;
	const char* imageFileLadder;
	const char* imageFileSnowman;
	const char* imageFilePlatform;
} Platform, Ladder, Snowman, Lives;

Platform platforms[MAX_PLATFORMS];
Ladder ladders[MAX_LADDERS];
Points points[MAX_POINTS];
Snowman snowman;
Lives lives[MAX_LIVES];


void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};



void ClearBoard() {
	for (int i = 9; i < MAX_PLATFORMS; i++) {
		platforms[i].V2.x = DELETE_FROM_BOARD;
		platforms[i].V2.y = 0;
	}
	for (int i = 0; i < MAX_LADDERS; i++) {
		ladders[i].V2.x = DELETE_FROM_BOARD;
		ladders[i].V2.y = 0;

		points[i].V2.x = DELETE_FROM_BOARD;
		points[i].V2.y = 0;
	}
	snowman.V2.x = 0;
	snowman.V2.y = 0;
}
void CleanSDL(Game& game) {
	SDL_FreeSurface(game.charset);
	SDL_FreeSurface(game.playerModel);
	SDL_FreeSurface(game.screen);
	SDL_DestroyTexture(game.scrtex);
	SDL_DestroyRenderer(game.renderer);
	SDL_DestroyWindow(game.window);
	SDL_Quit();
}
void hideAllPoints() {
	points->hidePoint1 = false;
	points->hidePoint2 = false;
	points->hidePoint3 = false;
}

void EveryPlatformInitialize(Game& game, int countAfterMainPlatform, int height, int start, int FirstCoord, bool inclinedPlatform, bool raiseToTheLeft) {
	for (int i = start; i < SPAWN_PLATFORM + countAfterMainPlatform; i++) {
		platforms[start].V2.x = SPAWN_PLATFORM_X + FirstCoord;
		platforms[i].V2.x = platforms[i - 1].V2.x - PLATFORM_LENGTH;
		if (!inclinedPlatform) {
			platforms[i].V2.y = SPAWN__PLATFORM_Y - height;
		}
		else {
			platforms[start].V2.y = SPAWN__PLATFORM_Y - height;
			if (!raiseToTheLeft) {
				platforms[i].V2.y = platforms[i - 1].V2.y + 5;
			}
			else {
				platforms[i].V2.y = platforms[i - 1].V2.y - 5;
			}
		}
	}
}

int InitializeGame(Game& game) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	game.rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &game.window, &game.renderer);
	if (game.rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(game.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(game.window, "Danylo Zherzdiev 196765");


	game.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	game.scrtex = SDL_CreateTexture(game.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);


	game.charset = SDL_LoadBMP("./cs8x8.bmp");
	game.playerModel = SDL_LoadBMP("./player1.bmp");

	if (game.charset == NULL || game.playerModel == NULL) {
		CleanSDL(game);
		return 1;
	};
	SDL_SetColorKey(game.charset, true, 0x000000);

	game.black = SDL_MapRGB(game.screen->format, 0x00, 0x00, 0x00);
	game.green = SDL_MapRGB(game.screen->format, 0x00, 0xFF, 0x00);
	game.red = SDL_MapRGB(game.screen->format, 0xFF, 0x00, 0x00);
	game.blue = SDL_MapRGB(game.screen->format, 0x11, 0x11, 0xCC);

	game.timer1 = SDL_GetTicks();
}
void InitializePlatforms(Player& player, Game& game, int red) {
	for (int i = 0; i < SPAWN_PLATFORM; i++) {
		platforms[0].V2.x = SPAWN_PLATFORM_X;
		platforms[i].V2.x = platforms[i - 1].V2.x + PLATFORM_LENGTH;
		platforms[i].V2.y = SPAWN__PLATFORM_Y;
	}
	if (game.Level1 == true) {
			printf("You're on the FIRST level!\n");

			EveryPlatformInitialize(game, 4, 100, 9, 1050, false, false);
			EveryPlatformInitialize(game, 6, 100, 12, 450, false, false);
			EveryPlatformInitialize(game, 9, 300, 15, 1050, false, false);
			EveryPlatformInitialize(game, 14, 500, 18, 450, false, false);
			EveryPlatformInitialize(game, 18, 400, 23, 1200, false, false);
			EveryPlatformInitialize(game, 20, 200, 27, 50, false, false);

			EveryPlatformInitialize(game, 24, 700, 29, 750, false, false);
			EveryPlatformInitialize(game, 26, 735, 33, 600, false, false);

			EveryPlatformInitialize(game, 28, 600, 35, 1200, false, false);

			game.IsUnevenPlatform = false;
			game.currentLevel = 1;
		}
		else if (game.Level2 == true) {
			printf("You're on the SECOND level!\n");

			EveryPlatformInitialize(game, 7, 150, 9, 1200, true, false);
			EveryPlatformInitialize(game, 14, 250, 16, 900, true, true);
			EveryPlatformInitialize(game, 21, 400, 23, 1200, true, false);
			EveryPlatformInitialize(game, 37, 600, 37, 1200, false, false);
			game.IsUnevenPlatform = true;
			game.currentLevel = 2;
		}
		else if (game.Level3 == true && !game.EndGame) {
			printf("You're on the THIRD level!\n");

			EveryPlatformInitialize(game, 4, 100, 9, 1200, false, false);
			EveryPlatformInitialize(game, 8, 300, 13, 400, false, false);
			EveryPlatformInitialize(game, 12, 500, 17, 1200, false, false);
			EveryPlatformInitialize(game, 16, 700, 21, 400, false, false);

			EveryPlatformInitialize(game, 37, 600, 37, 1200, false, false);
			game.IsUnevenPlatform = false;
			game.currentLevel = 3;
		}

	for (int i = 0; i < MAX_PLATFORMS; i++) {
		platforms[i].imageFilePlatform = "./grass.bmp";
	}
}
void InitializeLedder(Game game) {
	if (game.Level1 == true) {
		int coordinates[][2] = {
		{900, 914}, // spawn right
		{300, 914}, // spawn left

		{1050, 815}, // 
		{1050, 740}, // three on right 1
		{1050, 714}, // 
		
		{150, 815}, // leftmost

		{850, 615}, // second part third

		{1000, 515}, // two on the bottom on right
		{1000, 415}, // 

		{300, 415}, // two on the bottom before finish
		{300, 315} // 
		};
		for (int i = 10; i < MAX_LADDERS; i++) {
			ladders[i].V2.x = DELETE_FROM_BOARD;
			ladders[i].V2.y = 0;
		}
		for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
			ladders[i].V2.x = coordinates[i][0];
			ladders[i].V2.y = coordinates[i][1];
		}
	
	}
	else if (game.Level2 == true) {
		for (int i = 2; i < MAX_LADDERS; i++) {
			ladders[i].V2.x = DELETE_FROM_BOARD;
			ladders[i].V2.y = 0;
		}
		ladders[0].V2.x = 1250;
		ladders[0].V2.y = 514;
		ladders[1].V2.x = 1250;
		ladders[1].V2.y = 440;
		ladders[2].V2.x = 1250;
		ladders[2].V2.y = 414;
	}
	else if (game.Level3 == true) {
		int coordinates[][2] = {
		{750, 914}, // spawn right
		{750, 815},
		{750, 715},
		{750, 615},
		{750, 515},
		{750, 415},

		{550, 914}, // spawn right
		{550, 815},
		{550, 715},
		{550, 615},
		{550, 515},
		{550, 415},

		};
		for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
			ladders[i].V2.x = coordinates[i][0];
			ladders[i].V2.y = coordinates[i][1];
		}
	}

	for (int i = 0; i < MAX_LADDERS; i++) {
		ladders[i].imageFileLadder = "./ledder.bmp";
	}
}
void InitializeSnowman(Game& game) {
	if (game.Level1 == true) {
		snowman.V2.x = 575;
		snowman.V2.y = 205;
	}
	else if (game.Level2 == true) {
		snowman.V2.x = 650;
		snowman.V2.y = 340;
	}
	else if (game.Level3 == true) {
		snowman.V2.x = 650;
		snowman.V2.y = 340;
	}

	snowman.imageFileSnowman = "./snowman.bmp";
}
void InitializeLives() {
	for (int i = 0; i < lives->lives_count; i++) {
		lives[i].V2.x = 30 + (i * 55);
		lives[i].V2.y = 100;
		lives[i].imageFileLives = "./live.bmp";
	}
}
void InitializePoints(Game& game, Player& player) {
	if (points->isCollected == true) {
		points->pointsText.count += 50;
	}
	for (int i = 0; i < MAX_POINTS; i++) {
		if (game.EndLevel == false && game.EndGame == false) {
			if (game.Menu) { points[i].pointsText.V2.x = DELETE_FROM_BOARD; }
			else { points[i].pointsText.V2.x = player.V2.x; }
			points[i].pointsText.V2.y = player.V2.y - 40;

			if (points->pointsText.count >= 100) {
				points[i].pointsText.V2.x -= 5;
			}
			if (points[i].pointsText.count > 0){
				sprintf(game.text, "%d", points[i].pointsText.count);
				DrawString(game.screen, points[i].pointsText.V2.x - 5, points[i].pointsText.V2.y, game.text, game.charset);
			}
		}
		points[i].imageFilePoints = "./points.bmp";
	}
	if (game.Level1 == true) {
		for (int i = 3; i < MAX_LADDERS; i++) {
			points[i].V2.x = DELETE_FROM_BOARD;
			points[i].V2.y = 0;
		}
		if (points->hidePoint1 == true) {
			points[0].V2.x = DELETE_FROM_BOARD;
			points[0].V2.y = 0;
		}
		else if (points->hidePoint2 == true) {
			points[1].V2.x = DELETE_FROM_BOARD;
			points[1].V2.y = 0;
		}
		else if (points->hidePoint3 == true) {
			points[2].V2.x = DELETE_FROM_BOARD;
			points[2].V2.y = 0;
		}
		else if(game.Level1){
			points[0].V2.x = 50; points[0].V2.y = 740;

			points[1].V2.x = 1200; points[1].V2.y = 540;

			points[2].V2.x = 50; points[2].V2.y = 440;
		}
		else if(game.Level3){
			points[0].V2.x = 50; points[0].V2.y = 640;

			points[1].V2.x = 1200; points[1].V2.y = 840;

			points[2].V2.x = 50; points[2].V2.y = 240;
		}
		if (points->isCollected == true) {
			if (points->first == true) {
				points[0].V2.x = DELETE_FROM_BOARD;
				points[0].V2.y = 0;
				points->isCollected = false; points->hidePoint1 = true; points->first = false;
			}
			else if (points->second == true) {
				points[1].V2.x = DELETE_FROM_BOARD;
				points[1].V2.y = 0;
				points->isCollected = false; points->hidePoint2 = true; points->second = false;
			}
			else if (points->third == true) {
				points[2].V2.x = DELETE_FROM_BOARD;
				points[2].V2.y = 0;
				points->isCollected = false; points->hidePoint3 = true; points->third = false;
			}
		}
	}
}
void InitializePlayer(SDL_Surface* playerModel, Player& player) {
	player.V2.x = START_X;
	player.V2.y = START_Y;
	player.width = playerModel->w;
	player.height = playerModel->h;
	player.speed = 0.0;
	player.isJumping = false;
	player.jumpStartY = 0.0;
	player.jumpVelocity = 0.0;
}
void InitializeAll(Game& game, Player& player, int red) {
	InitializePlatforms(player, game, red);
	InitializeLedder(game);
	InitializeSnowman(game);
}

void DrawPoints(SDL_Surface* screen) {
	for (int i = 0; i < MAX_POINTS; i++) {
		SDL_Surface* pointsImage = SDL_LoadBMP(points[i].imageFilePoints);
		if (pointsImage == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", points[i].imageFilePoints, SDL_GetError());
			continue;
		}
		DrawSurface(screen, pointsImage, points[i].V2.x, points[i].V2.y);
		SDL_FreeSurface(pointsImage);
	}
}
void DrawLives(SDL_Surface* screen) {
	for (int i = 0; i < lives->lives_count; i++) {
		SDL_Surface* livesImage = SDL_LoadBMP(lives[i].imageFileLives);
		if (livesImage == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", lives[i].imageFileLives, SDL_GetError());
			continue;
		}
		DrawSurface(screen, livesImage, lives[i].V2.x, lives[i].V2.y);
		SDL_FreeSurface(livesImage);
	}
}
void DrawLedder(SDL_Surface* screen) {
	for (int i = 0; i < MAX_PLATFORMS; i++) {
		SDL_Surface* platformImage = SDL_LoadBMP(ladders[i].imageFileLadder);
		if (platformImage == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", ladders[i].imageFileLadder, SDL_GetError());
			continue;
		}
		DrawSurface(screen, platformImage, ladders[i].V2.x, ladders[i].V2.y);
		SDL_FreeSurface(platformImage);
	}
}
void DrawSnowman(SDL_Surface* screen) {
	SDL_Surface* snowmanImage = SDL_LoadBMP(snowman.imageFileSnowman);
	if (snowmanImage == NULL) {
		printf("SDL_LoadBMP(%s) error: %s\n", snowman.imageFileSnowman, SDL_GetError());
	}
	DrawSurface(screen, snowmanImage, snowman.V2.x, snowman.V2.y);
	SDL_FreeSurface(snowmanImage);
}
void DrawPlatforms(Game& game, Player& player) {
	for (int i = 0; i < MAX_PLATFORMS; i++) {
		SDL_Surface* platformImage = SDL_LoadBMP(platforms[i].imageFilePlatform);
		if (platformImage == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", platforms[i].imageFilePlatform, SDL_GetError());
			continue;
		}
		DrawSurface(game.screen, platformImage, platforms[i].V2.x, platforms[i].V2.y);
		SDL_FreeSurface(platformImage);
	}
}
void DrawAll(SDL_Surface* screen, Game& game, Player& player) {
	DrawPlatforms(game, player);
	DrawLedder(screen);
	DrawSnowman(screen);
}

bool IsPlayerCollidingWithPlatform(Player& player, PlatformData& platform) {
	if (player.V2.x < platform.V2.x + platform.width - 50 &&
		player.V2.x + player.width + 50 > platform.V2.x &&
		player.V2.y + player.height > platform.V2.y && player.V2.y >= platform.V2.y - 40 && player.V2.y <= platform.V2.y - 30) {
		return true;
	}
	return false;
}
int CheckCollision(Player* player, Platform* platform, Ladder* ladders) {
	if (player->V2.x < LEFT_BORDER_PLATFORM) {
		player->leftBorder = true;
		return 1;
	}
	else if (player->V2.x > RIGHT_BORDER_PLATFORM) {
		player->rightBorder = true;
		return 1;
	}

	if (player->V2.x <= ladders->V2.x && player->V2.x >= ladders->V2.x) {
		if (player->V2.y >= ladders->V2.y - TOP_OF_LADDER && player->V2.y <= ladders->V2.y + TOP_OF_LADDER) {
			player->ItsLadderHereUp = true;
			if (player->V2.y <= ladders->V2.y + 25) {
				player->ItsLadderHereDown = true;
			}
		}
		return 1;
	}
	return 0;
}

void FixSmallSolution(Player& player, int levelMin, int levelMax) {
	if (player.V2.y == levelMin) {
	}
	else if (player.V2.y > levelMin && player.V2.y < levelMax) {
		player.V2.y = levelMin;
	}
	player.verticalVelocity = 0.0;
}

void Move(Player* player, Game* game) {
	if (player->isMovingLeft && (!game->EndLevel && !game->EndGame && !game->Menu)) {
		player->V2.x -= player->speed;
	}
	else if (player->isMovingRight && (!game->EndLevel && !game->EndGame && !game->Menu))
	{
		player->V2.x += player->speed;
	}
	else if((player->isMovingRight || player->isMovingLeft) && (game->EndLevel || game->EndGame)){
		printf("Please start a new game or next level to move!\n");
		player->isMovingRight = false;
		player->isMovingLeft = false;
	}
}
void Jump(Player* player, float delta) {
	if (player->isJumping && player->V2.y >= 100) {
		player->jumpVelocity = JUMP_HEIGHT + (GRAVITY * delta);
		player->V2.y -= player->jumpVelocity * delta;
		if (player->V2.y <= 0.0) {
			player->isJumping = false;
			player->jumpVelocity = 0.0;
			player->V2.y = 0.0;
		}
	}
}
void FinishCollision(Game* game, Player* player) {
	if (player->V2.x == snowman.V2.x && player->V2.y == snowman.V2.y) {
		if (game->currentLevel <= 2) {
			game->EndLevel = true;
			points->pointsText.count += 100;
			for (int i = 0; i < 1; i++) {
				points->hidePoint1 = true;
				points->hidePoint2 = true;
				points->hidePoint3 = true;
			}
			player->isMovingLeft = false; player->isMovingRight = false;
			game->Level3 = false;
			printf("\nCongratulations, you finished this level! You got 100 more points!\n");
			printf("Your score: %d\n\n", points->pointsText.count);
		}
		else {
			game->EndGame = true;
			points->pointsText.count += 200;
			printf("\nCongratulations, you finished the GAME!\n");
			printf("Your score: %d\n\n", points->pointsText.count);
		}
	}
}
void PointsCollision(Player* player) {
	for (int i = 0; i < MAX_POINTS; i++) {
		if (player->V2.x >= points[i].V2.x && player->V2.x <= points[i].V2.x && player->V2.y == points[i].V2.y) {
			points->isCollected = true;
			for (int j = 0; j < 3; j++) {
				points->first = (j == 0 && player->V2.x == points[j].V2.x && player->V2.y == points[j].V2.y);
				points->second = (j == 1 && player->V2.x == points[j].V2.x && player->V2.y == points[j].V2.y);
				points->third = (j == 2 && player->V2.x == points[j].V2.x && player->V2.y == points[j].V2.y);

				if (points->first || points->second || points->third) break;
			}
			printf("\nYou found a coin and get 50 points!\n");
		}
	}
}
void isNotCollisionPlatform(Game* game, Player* player, float delta) {
	if (!player->playerColliding) {

	
		player->verticalVelocity += GRAVITY * delta;
		player->V2.y += player->verticalVelocity * delta;

		for (int i = 0; i < MAX_PLATFORMS; i++) {
			PlatformData platformData = { platforms[i].V2.x, platforms[i].V2.y, PLATFORM_LENGTH };
			if (IsPlayerCollidingWithPlatform(*player, platformData)) {
				if (game->IsUnevenPlatform == false) {
					for (int i = 840; i >= 240; i -= 100) {
						if (player->V2.y >= i) { FixSmallSolution(*player, i, i + 100); break; }
					}
					if (player->V2.y >= 205) { FixSmallSolution(*player, 205, 240); }
				}
				else {
					if (player->V2.y <= 840 && player->V2.y >= 440) { FixSmallSolution(*player, 840, 940); }
					else if (player->V2.y >= 340) { FixSmallSolution(*player, 340, 440); }
				}
			}
		}
		if (player->V2.y >= 940) {
			player->V2.y = 940;
			printf("Player fell on the spawn board!\n");
			player->verticalVelocity = 0.0;
		}
	}
	else {
		for (int i = 9; i < 30; i++) {
			PlatformData platformData = { platforms[i].V2.x, platforms[i].V2.y, PLATFORM_LENGTH };
			if (IsPlayerCollidingWithPlatform(*player, platformData) && game->IsUnevenPlatform == true) {
				if (platforms[i - 1].V2.y >= player->V2.y - 20 && player->V2.x == platforms[i].V2.x + 70) {
					player->V2.y -= 5;
				}
			}
		}
	}
}
void LogicCollisionWithBordersAndLadders(Game* game, Player* player) {
	for (int i = 0; i < MAX_PLATFORMS; i++) {
		if (CheckCollision(player, &platforms[i], &ladders[i])) {
			if (player->leftBorder == true) {
				player->V2.x += 10; player->leftBorder = false;
			}
			else if (player->rightBorder == true) {
				player->V2.x -= 10; player->rightBorder = false;
			}
			else if (player->ItsLadderHereUp == true && player->CanMoveUp == true) {
				player->V2.y -= 10;
				player->ItsLadderHereUp = false; player->CanMoveUp = false;
			}
			else if (player->ItsLadderHereDown == true && player->CanMoveDown == true) {
				player->V2.y += 10;
				player->ItsLadderHereDown = false; player->CanMoveDown = false;
			}
		}
	}
	if (player->V2.y <= 240 && player->V2.y >= 200 && player->V2.x == 405 && game->Level1 == true) {
		if (player->isMovingRight) {
			player->V2.x -= 5;
		}
	}
	else if (player->V2.y <= 240 && player->V2.y >= 200 && player->V2.x == 745 && game->Level1 == true) {
		if (player->isMovingLeft) {
			player->V2.x += 5;
		}
	}
}
void isEndLevelOrGame(Game* game) {
	if (game->EndLevel == true) {
		ClearBoard();
		if (game->currentLevel == 1) {
			if (game->Level2 == false) {
				game->Level2 = true;
			}
		}
		else if (game->currentLevel == 2) {
			if (game->Level3 == false && game->Level2 == true) {
				game->Level3 = true;
			}
		}
	}
	else if (game->EndGame == true) {
		ClearBoard();
	}
}

void UpdatePlayer(Player* player, Platform* platform, Points* points, Game* game)
{
	player->playerColliding = false;

	Move(player, game);
	Jump(player, game->delta);
	FinishCollision(game, player);
	PointsCollision(player);
	isEndLevelOrGame(game);
	LogicCollisionWithBordersAndLadders(game, player);

	for (int j = 0; j < MAX_PLATFORMS; j++) {
		PlatformData platformData = { platforms[j].V2.x, platforms[j].V2.y, PLATFORM_LENGTH };

		if (IsPlayerCollidingWithPlatform(*player, platformData)) {
			player->playerColliding = true;
			break;
		}
		else if (player->V2.x <= ladders[j].V2.x && player->V2.x >= ladders[j].V2.x && !player->isJumping) {
			player->playerColliding = true;
			break;
		}
	}
	isNotCollisionPlatform(game, player, game->delta);
}

void SaveHighScore(Game& game, const char* nickname, int score) {
	FILE* file = fopen("high_scores.txt", "a");
	if (file != NULL) {
		fprintf(file, "%s %d\n", inputText, score);
		fclose(file);
		printf("Your score saved!\n\n");
	}
}

void GetNicknameSDL(Game& game) {
	while (SDL_PollEvent(&game.event) != 0) {
		switch (game.event.type) {
		case SDL_QUIT:
			game.quit = 1;
			break;
		case SDL_KEYDOWN:
			if (game.event.key.keysym.sym == SDLK_RETURN) {
				SaveHighScore(game, game.nickname, points->pointsText.count);
				memset(inputText, 0, sizeof(inputText));
				game.Menu = true;
				game.isEnterNickname = false;
				game.canWrite = false;
			}
			else if (game.event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
				inputText[strlen(inputText) - 1] = '\0';
			}
			else if (strlen(inputText) < sizeof(inputText) - 1) {
				char key = game.event.key.keysym.sym;
				if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key == ' ') {
					strncat(inputText, &key, 1);
				}
			}
		};
	}
	DrawString(game.screen, 100, 150, "Enter your nickname:", game.charset);
	DrawString(game.screen, 100, 170, inputText, game.charset);
}


int CompareScores(const void* a, const void* b) {
	return ((Game*)b)->score - ((Game*)a)->score;
}

void DisplayHighScores(Game& game) {
	FILE* file = fopen("high_scores.txt", "r");
	if (file != NULL) {
		int lineNumber = 0;
		Game highScores[256];

		while (fscanf(file, "%s %d\n", highScores[lineNumber].nickname, &highScores[lineNumber].score) != EOF && lineNumber < 256) {
			lineNumber++;
		}
		fclose(file);

		qsort(highScores, lineNumber, sizeof(Game), CompareScores);

		int startLine = (game.currentPage - 1) * 10;
		int endLine = startLine + 10;

		for (int i = startLine; i < lineNumber && i < endLine; i++) {
			char displayText[256];
			sprintf(displayText, "%s: %d", highScores[i].nickname, highScores[i].score);
			DrawString(game.screen, game.screen->w / 2 - strlen(displayText) * 8 / 2, 300 + ((i - startLine) * 50), displayText, game.charset);
		}

		if (game.goToNextPage == true && lineNumber >= endLine) {
			game.currentPage++;
			game.goToNextPage = false;
		}
		if (game.goToPreviousPage == true && game.currentPage > 1) {
			game.currentPage--;
			game.goToPreviousPage = false;
		}
		if (game.currentPage > 1) {
			DrawString(game.screen, game.screen->w / 2 + 30, 830, "Press 'P' for previous page", game.charset);
		}
		if (lineNumber >= endLine) {
			DrawString(game.screen, game.screen->w / 2 - 11 * 20, 830, "Press 'N' for next page", game.charset);
		}
	}
}

void DrawMenu(Game& game) {
	DrawRectangle(game.screen, 0, 1, SCREEN_WIDTH, 999, game.blue, game.black); // BACKGROUND

	DrawRectangle(game.screen, 480, 0, SCREEN_WIDTH / 4, 15, game.blue, game.blue); // 
	sprintf(game.text, "POINTS (A,B,D,F,G,H) COMPLITED!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 5, game.text, game.charset);
	
	DrawRectangle (game.screen, 380, 200, 30, 150, game.blue, game.blue); DrawRectangle(game.screen, 390, 200, 150, 30, game.blue, game.blue); //		M
	DrawRectangle(game.screen, 440, 190, 50, 30, game.black, game.black); DrawRectangle(game.screen, 510, 200, 30, 150, game.blue, game.blue); //

	DrawRectangle(game.screen, 560, 200, 30, 150, game.blue, game.blue); DrawRectangle(game.screen, 560, 200, 75, 30, game.blue, game.blue);   //		E
	DrawRectangle(game.screen, 560, 260, 75, 30, game.blue, game.blue); DrawRectangle(game.screen, 560, 320, 75, 30, game.blue, game.blue);    //

	DrawRectangle(game.screen, 660, 200, 100, 150, game.blue, game.blue); DrawRectangle(game.screen, 690, 285, 25, 65, game.black, game.black);//		N
	DrawRectangle(game.screen, 705, 200, 25, 65, game.black, game.black); DrawRectangle(game.screen, 705, 250, 10, 50, game.blue, game.blue);  //

	DrawRectangle(game.screen, 790, 200, 30, 150, game.blue, game.blue); //
	DrawRectangle(game.screen, 790, 320, 100, 30, game.blue, game.blue); // U
	DrawRectangle(game.screen, 860, 200, 30, 150, game.blue, game.blue); //
	
	DrawRectangle(game.screen, 490, 500, 300, 30, game.blue, game.blue); // 
	sprintf(game.text, "NEW GAME - 'N'"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 510, game.text, game.charset);

	DrawRectangle(game.screen, 490, 600, 300, 30, game.blue, game.blue); // 
	sprintf(game.text, "RECORDS - 'R'"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 610, game.text, game.charset);

	DrawRectangle(game.screen, 405, 700, 150, 30, game.blue, game.blue); // left
	DrawRectangle(game.screen, 565, 700, 150, 30, game.blue, game.blue); // center
	DrawRectangle(game.screen, 725, 700, 150, 30, game.blue, game.blue); // right
	sprintf(game.text, "  LEVEL ONE - '1'     LEVEL TWO - '2'    LEVEL THREE - '3'"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 710, game.text, game.charset);

	DrawRectangle(game.screen, 565, 800, 150, 30, game.blue, game.blue); // center
	sprintf(game.text, "EXIT - 'esc'"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 810, game.text, game.charset);
}
void DrawRecords(Game& game) {
	DrawRectangle(game.screen, 0, 1, SCREEN_WIDTH, 999, game.blue, game.black);

	DrawRectangle(game.screen, 400, 200, 500, 600, game.blue, game.black);
	DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 210, "BACK - 'M'", game.charset);

	DisplayHighScores(game);
}
void TextDraw(Game& game, Player& player) {
	if (game.Menu == true) {
		ClearBoard();	
		DrawMenu(game);
		player.V2.x = 0;
		player.V2.y = 940;
		points->pointsText.count = 0;
		game.EndGame = false;
		game.isEnterNickname = false;
		game.canWrite = false;
	}
	else if (game.Records == true) {
		DrawRecords(game);
		player.V2.x = 0;
		player.V2.y = 940;
	}
	else {
		DrawRectangle(game.screen, 0, 4, SCREEN_WIDTH, 70, game.red, game.blue);

		sprintf(game.text, "Zherzdiev Danylo 196765, time = %.1lf s  %.0lf fps", game.worldTime, game.fps); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 10, game.text, game.charset);
		sprintf(game.text, "Esc - exit, \032 \030 \033 - move, Menu - 'M'"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 26, game.text, game.charset);
		sprintf(game.text, "Choose Level: (1) Level 1; (2) Level 2; (3) Level 3;"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 42, game.text, game.charset);
		sprintf(game.text, "X: %d | Y: %d", player.V2.x, player.V2.y); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 93, game.text, game.charset);
		sprintf(game.text, "Current Level: %d", game.currentLevel); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 77, game.text, game.charset);

		if (game.EndLevel == true || game.EndGame == true) {
			if (game.EndLevel == true) {
				sprintf(game.text, "Congratulations, you finished this level! You got 100 more points!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 452, game.text, game.charset);
				sprintf(game.text, "Press - 'L' to play next level!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 500, game.text, game.charset);
			}
			else if (game.EndGame == true) {
				if (game.canWrite == true) {
					ClearBoard();
					game.isEnterNickname = true;
					GetNicknameSDL(game);
				}
				sprintf(game.text, "Congratulations, you finished the game!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 452, game.text, game.charset);
				sprintf(game.text, "Press - 'W' to save your score and write your nickname!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 500, game.text, game.charset);
			}
			sprintf(game.text, "Your score: %d", points->pointsText.count); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 468, game.text, game.charset);
			sprintf(game.text, "Press - 'N' to start a new game!"); DrawString(game.screen, game.screen->w / 2 - strlen(game.text) * 8 / 2, 516, game.text, game.charset);
		}
	}
}


void handleHorizontalMovement(Game& game, Player& player) {
	player.isMovingLeft = (game.event.key.keysym.sym == SDLK_LEFT);
	player.isMovingRight = (game.event.key.keysym.sym == SDLK_RIGHT);
	player.speed = 5.0;
}
void handleVerticalMovement(Game& game, Player& player) {
	player.CanMoveUp = (game.event.key.keysym.sym == SDLK_UP);
	player.CanMoveDown = (game.event.key.keysym.sym == SDLK_DOWN);
}
void handleJump(Game& game, Player& player) {
	if (!player.isJumping && !game.EndLevel && !game.EndGame) {
		player.isJumping = true;
		player.jumpStartY = player.V2.y;
	}
}
void handleMenu(Game& game) {
	ClearBoard();
	points->hidePoint1 = true; points->hidePoint2 = true; points->hidePoint3 = true;
	game.Menu = true;
	game.Records = false;
	game.EndGame = false;
	points->pointsText.count = 0;
	game.currentPage = 1;
}
void handleRecords(Game& game) {
	if (game.Menu == true) {
		ClearBoard();
		game.Menu = false;
		game.Records = true;
	}
}
void handleNewGame(Game& game, Player& player) {
	if (game.Records == false) {
		ClearBoard();
		player.V2.x = START_X; player.V2.y = START_Y;
		game.Level2 = false; game.Level1 = true; game.Level3 = false;
		game.Menu = false; game.EndLevel = false; game.EndGame = false;

		hideAllPoints();

		game.worldTime = 0;
		points->pointsText.count = 0;
		lives->lives_count = 3;

		InitializeAll(game, player, game.red);
		InitializePoints(game, player);
		printf("New Game has been started!\n\n");
	}
	else if (game.Records == true) {
		game.goToNextPage = true;
	}
}
void handleLevel(SDL_Keycode keycode, Game& game, Player& player) {
	ClearBoard();
	player.V2.x = START_X; player.V2.y = START_Y;
	game.Menu = false;
	game.Records = false;
	game.Level1 = (keycode == SDLK_1); 
	game.Level2 = (keycode == SDLK_2);
	game.Level3 = (keycode == SDLK_3);
	game.currentLevel = 2;
	hideAllPoints();
	InitializeAll(game, player, game.red);
	InitializePoints(game, player);
}
void handleNextLevel(Game& game, Player& player) {
	ClearBoard();
	player.V2.x = START_X; player.V2.y = START_Y;
	game.Level1 = false;
	if (game.Level2 == true && game.currentLevel == 1) {
		game.currentLevel = 2;
		game.Level2 = true; game.Level3 = false;
	}
	else if (game.Level3 == true && game.currentLevel == 2) {
		game.currentLevel = 3;
		game.Level3 = true; game.Level2 = false;
	}
	game.EndLevel = false;
	InitializeAll(game, player, game.red);
}

void EventKeyDown(Game& game, Player& player) {
	if (game.isEnterNickname == false) {
		if (game.event.key.keysym.sym == SDLK_ESCAPE) game.quit = 1;
		else if (game.event.key.keysym.sym == SDLK_LEFT || game.event.key.keysym.sym == SDLK_RIGHT) {
			handleHorizontalMovement(game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_UP || game.event.key.keysym.sym == SDLK_DOWN) {
			handleVerticalMovement(game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_SPACE) {
			handleJump(game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_m) {
			handleMenu(game);
		}
		else if (game.event.key.keysym.sym == SDLK_r) {
			handleRecords(game);
		}
		else if (game.event.key.keysym.sym == SDLK_n) {
			handleNewGame(game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_p && game.Records == true) {
			game.goToPreviousPage = true;
		}
		else if ((game.event.key.keysym.sym == SDLK_1 || game.event.key.keysym.sym == SDLK_2 || game.event.key.keysym.sym == SDLK_3) && !game.Records) {
			handleLevel(game.event.key.keysym.sym, game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_l && (game.Level2 == true || game.Level3 == true) && !game.EndGame) {
			handleNextLevel(game, player);
		}
		else if (game.event.key.keysym.sym == SDLK_w) {
			game.canWrite = true;
		}
		else if (game.event.key.keysym.sym == SDLK_o) {
			lives->lives_count++;
			InitializeLives();
		}
		else if (game.event.key.keysym.sym == SDLK_i) {
			if (lives->lives_count > 0) {
				lives->lives_count--;
				InitializeLives();
			}
			else {
				handleMenu(game);
			}
		}
	}
}
void EventTextInput(Game& game) {
	if (strlen(inputText) < MAX_NICKNAME_LENGTH && game.isEnterNickname == true) {
		strcat(inputText, game.event.text.text);
	}
}
void EventKeyUp(Player& player) {
	player.speed = 0.0;
	player.CanMoveUp = false; player.CanMoveDown = false;
	player.isJumping = false;
}
void EventKeyLogic(Game& game, Player& player) {
	while (SDL_PollEvent(&game.event)) {
		switch (game.event.type) {
		case SDL_KEYDOWN:
			EventKeyDown(game, player);
			break;
		case SDL_TEXTINPUT:
			EventTextInput(game);
			break;
		case SDL_KEYUP:
			EventKeyUp(player);
			break;
		case SDL_QUIT:
			game.quit = 1;
			break;
		};
	}
	game.frames++;
}

void TimeLogic(Game& game) {
	game.timer2 = SDL_GetTicks();
	game.delta = (game.timer2 - game.timer1) * 0.001;
	game.timer1 = game.timer2;

	game.worldTime += game.delta;

	game.fpsTimer += game.delta;
	if (game.fpsTimer > SECONDS_BETWEEN_REFRESH) {
		game.fps = game.frames * REFRESH_RATE;
		game.frames = 0;
		game.fpsTimer -= SECONDS_BETWEEN_REFRESH;
	};
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	Game game;
	Player player;

	InitializeGame(game);

	InitializePlayer(game.playerModel, player);
	InitializeAll(game, player, game.red);
	InitializeLives();

	while (!game.quit) {
		SDL_FillRect(game.screen, NULL, game.black);
		TimeLogic(game);
		UpdatePlayer(&player, platforms, points, &game);

		DrawLives(game.screen);
		DrawAll(game.screen, game, player);
		DrawSurface(game.screen, game.playerModel, player.V2.x, player.V2.y);
		TextDraw(game, player);

		InitializePoints(game, player);
		DrawPoints(game.screen);

		SDL_UpdateTexture(game.scrtex, NULL, game.screen->pixels, game.screen->pitch);
		SDL_RenderClear(game.renderer);
		SDL_RenderCopy(game.renderer, game.scrtex, NULL, NULL);
		SDL_RenderPresent(game.renderer);

		EventKeyLogic(game, player);
	}

	CleanSDL(game);
	return 0;
};
