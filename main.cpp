#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "stdbool.h"

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

#define COORD_SPAWN_Y		  980
#define COORD_SPAWN_X		  50

#define PLATFORM_LENGTH       150

#define LEFT_BORDER_PLATFORM  21
#define RIGHT_BORDER_PLATFORM 1259

#define START_X               650
#define START_Y               840

#define GRAVITY				  400
#define JUMP_HEIGHT			  250.0

#define TOP_OF_LADDER         65

#define SECONDS_BETWEEN_REFRESH 0.5
#define REFRESH_RATE 1/SECONDS_BETWEEN_REFRESH


struct Vector2 {
	int x;
	int y;
};

struct Game {
	int timer1, timer2, quit = 0, frames = 0, rc;
	double delta, worldTime = 0, fpsTimer = 0, fps = 0, distance, etiSpeed;
	int currentLevel = 0;

	bool Level1 = false;
	bool Level2 = false;
	bool Level3 = false;

	bool IsUnevenPlatform = false;

	bool EndLevel = false;
	bool EndGame = false;

	bool canWrite = false;
};

struct PointsText {
	Vector2 V2;

	int count;
};

struct Points {
	Vector2 V2;
	PointsText pointsText;

	bool isCollected = false;
	bool first = false;
	bool second = false;
	bool third = false;

	bool hidePoint1 = false;
	bool hidePoint2 = false;
	bool hidePoint3 = false;

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
	int width;
	int height;
	int speed;

	bool isMovingLeft;
	bool isMovingRight;

	bool leftBorder;
	bool rightBorder;

	bool ItsLadderHere;
	bool ItsLadderHereDown;
	bool isOnLadder = false;

	bool CanMoveUp;
	bool CanMoveDown;

	bool isJumping;

	double verticalVelocity = 0.0;
	double jumpStartY;
	double jumpVelocity;
};

typedef struct {
	Vector2 V2;
	SDL_Surface* image;

	const char* imageFileLadder;
	const char* imageFileSnowman;
	const char* imageFilePlatform;
} Platform, Ladder, Snowman;

struct HighScore {
	char pseudonym[20];
	int score;
};

Platform platforms[MAX_PLATFORMS];
Ladder ladders[MAX_LADDERS];
Points points[MAX_POINTS];
Snowman snowman;


void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
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


void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
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
		platforms[i].V2.x = 0;
		platforms[i].V2.y = 0;
	}
	for (int i = 0; i < MAX_LADDERS; i++) {
		ladders[i].V2.x = 0;
		ladders[i].V2.y = 0;

		points[i].V2.x = 0;
		points[i].V2.y = 0;
	}
	snowman.V2.x = 0;
	snowman.V2.y = 0;
}


void EveryPlatformInitialize(Game& game, int countAfterMainPlatform, int height, int start, int FirstCoord, bool inclinedPlatform, bool raiseToTheLeft) {
	for (int i = start; i < SPAWN_PLATFORM + countAfterMainPlatform; i++) {
		platforms[start].V2.x = COORD_SPAWN_X + FirstCoord;
		platforms[i].V2.x = platforms[i - 1].V2.x - PLATFORM_LENGTH;
		if (!inclinedPlatform) {
			platforms[i].V2.y = COORD_SPAWN_Y - height;
		}
		else {
			platforms[start].V2.y = COORD_SPAWN_Y - height;
			if (!raiseToTheLeft) {
				platforms[i].V2.y = platforms[i - 1].V2.y + 5;
			}
			else {
				platforms[i].V2.y = platforms[i - 1].V2.y - 5;
			}
		}
	}
}

void InitializePlatforms(SDL_Surface* screen, Player& player, Game& game, int red) {
	for (int i = 0; i < SPAWN_PLATFORM; i++) {
		platforms[0].V2.x = COORD_SPAWN_X;
		platforms[i].V2.x = platforms[i - 1].V2.x + PLATFORM_LENGTH;
		platforms[i].V2.y = COORD_SPAWN_Y;
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
		//EveryPlatformInitialize(game, 28, 500, 30, 900, true, false);
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
			ladders[i].V2.x = -50;
			ladders[i].V2.y = 0;
		}
		for (int i = 0; i < sizeof(coordinates) / sizeof(coordinates[0]); i++) {
			ladders[i].V2.x = coordinates[i][0];
			ladders[i].V2.y = coordinates[i][1];
		}
	
	}
	else if (game.Level2 == true) {
		for (int i = 2; i < MAX_LADDERS; i++) {
			ladders[i].V2.x = -50;
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

void InitializePoints(SDL_Surface* screen, SDL_Surface* charset, Game& game, Player& player, char text[]) {
	if (points->isCollected == true) {
		points->pointsText.count += 50;
	}
	for (int i = 0; i < MAX_POINTS; i++) {
		if (game.EndLevel == false && game.EndGame == false) {
			points[i].pointsText.V2.x = player.V2.x;
			points[i].pointsText.V2.y = player.V2.y - 40;

			if (points->pointsText.count >= 100) {
				points[i].pointsText.V2.x -= 5;
			}
			if (points[i].pointsText.count > 0){
				sprintf(text, "%d", points[i].pointsText.count);
				DrawString(screen, points[i].pointsText.V2.x - 5, points[i].pointsText.V2.y, text, charset);
			}
			//printf("%d %d", points.x, points.y);
		}
		points[i].imageFilePoints = "./points.bmp";
	}
	if (game.Level1 == true) {
		for (int i = 3; i < MAX_LADDERS; i++) {
			points[i].V2.x = -50;
			points[i].V2.y = 0;
		}
		if (points->hidePoint1 == true) {
			points[0].V2.x = -50;
			points[0].V2.y = 0;
		}
		else if (points->hidePoint2 == true) {
			points[1].V2.x = -50;
			points[1].V2.y = 0;
		}
		else if (points->hidePoint3 == true) {
			points[2].V2.x = -50;
			points[2].V2.y = 0;
		}
		else {
			points[0].V2.x = 50;
			points[0].V2.y = 740;

			points[1].V2.x = 1200;
			points[1].V2.y = 540;

			points[2].V2.x = 50;
			points[2].V2.y = 440;
		}
		if (points->isCollected == true) {
			if (points->first == true) {
				points[0].V2.x = -50;
				points[0].V2.y = 0;
				points->isCollected = false;
				points->hidePoint1 = true;
				points->first = false;
			}
			else if (points->second == true) {
				points[1].V2.x = -50;
				points[1].V2.y = 0;
				points->isCollected = false;
				points->hidePoint2 = true;
				points->second = false;
			}
			else if (points->third == true) {
				points[2].V2.x = -50;
				points[2].V2.y = 0;
				points->isCollected = false;
				points->hidePoint3 = true;
				points->third = false;
			}
		}
	}
	else if (game.Level3) {

	}
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
void DrawPlatforms(SDL_Surface* screen, Game& game, Player& player, float delta) {
	for (int i = 0; i < MAX_PLATFORMS; i++) {
		SDL_Surface* platformImage = SDL_LoadBMP(platforms[i].imageFilePlatform);
		if (platformImage == NULL) {
			printf("SDL_LoadBMP(%s) error: %s\n", platforms[i].imageFilePlatform, SDL_GetError());
			continue;
		}
		DrawSurface(screen, platformImage, platforms[i].V2.x, platforms[i].V2.y);
		SDL_FreeSurface(platformImage);
	}
}

void DrawParallelLine(SDL_Surface* screen, PlatformData& platformData, Uint32 color) {
	DrawRectangle(screen, platformData.V2.x - 75, platformData.V2.y - 20, platformData.width, 20, color, color);
}

bool IsPlayerCollidingWithPlatform(Player& player, PlatformData& platform) {
	if (player.V2.x < platform.V2.x + platform.width - 50 &&
		player.V2.x + player.width + 50 > platform.V2.x &&
		player.V2.y + player.height > platform.V2.y && player.V2.y >= platform.V2.y - 40 && player.V2.y <= platform.V2.y - 30) {
		return true;
	}
	return false;
}

void FixSmallSolution(Player& player, int levelMin, int levelMax) {
	if (player.V2.y == levelMin) {
		printf("Player fell!\n");
	}
	else if (player.V2.y > levelMin && player.V2.y < levelMax) {
		player.V2.y = levelMin;
		printf("Player fell!\n");
	}
	player.verticalVelocity = 0.0;
}

int CheckCollision(Player* player, Platform* platform, Ladder* ladders, float delta) {
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
			player->ItsLadderHere = true;
			if (player->V2.y <= ladders->V2.y + 25) {
				player->ItsLadderHereDown = true;
			}
		}
		return 1;
	}
	return 0;
}

void Move(Player* player, Game* game) {
	if (player->isMovingLeft && (!game->EndLevel && !game->EndGame)) {
		player->V2.x -= player->speed;
	}
	else if (player->isMovingRight && (!game->EndLevel && !game->EndGame))
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

void UpdatePlayer(SDL_Surface* screen, Player* player, Points* points, Game* game, float delta)
{
	Uint32 lineColor = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	Move(player, game);
	Jump(player, delta);

	if (player->V2.x == snowman.V2.x && player->V2.y == snowman.V2.y) {
		if (game->currentLevel <= 2) {
			game->EndLevel = true;
			points->pointsText.count += 100;
			for (int i = 0; i < 1; i++) {
				points->hidePoint1 = true;
				points->hidePoint2 = true;
				points->hidePoint3 = true;
			}
			player->isMovingLeft = false;
			player->isMovingRight = false;
			game->Level3 = false;
			//InitializePoints(screen, charset, game, player, text);
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

	bool playerColliding = false;

	for (int j = 0; j < 37; j++) {
		PlatformData platformData = { platforms[j].V2.x, platforms[j].V2.y, PLATFORM_LENGTH };
		//DrawParallelLine(screen, platformData, lineColor);

		if (IsPlayerCollidingWithPlatform(*player, platformData)) {
			playerColliding = true;
			break;
		}
		else if (player->V2.x <= ladders[j].V2.x && player->V2.x >= ladders[j].V2.x && !player->isJumping) {
			playerColliding = true;
			break;
		}
	}
	if (player->V2.y <= 240 && player->V2.y >= 200 && player->V2.x == 405) {
		if (player->isMovingRight) {
			player->V2.x -= 5;
		}
	}
	else if (player->V2.y <= 240 && player->V2.y >= 200 && player->V2.x == 745) {
		if (player->isMovingLeft) {
			player->V2.x += 5;
		}
	}
	if (!playerColliding) {

		player->verticalVelocity += GRAVITY * delta;
		player->V2.y += player->verticalVelocity * delta;

		for (int i = 0; i < MAX_PLATFORMS; i++) {
			PlatformData platformData = { platforms[i].V2.x, platforms[i].V2.y, PLATFORM_LENGTH };
			if (IsPlayerCollidingWithPlatform(*player, platformData)) {
				if (game->IsUnevenPlatform == false) {
					for (int i = 840; i >= 240; i -= 100) {
						if (player->V2.y >= i) {
							FixSmallSolution(*player, i, i + 100);
							break;
						}
					}
					if (player->V2.y >= 205) {
						FixSmallSolution(*player, 205, 240);
					}
				}
				else {
					if (player->V2.y <= 840) {
						FixSmallSolution(*player, 840, 940);
					}
					else if (player->V2.y <= 340) {
						game->IsUnevenPlatform = false;
					}
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
		for (int i = 9; i < MAX_PLATFORMS; i++) {
			PlatformData platformData = { platforms[i].V2.x, platforms[i].V2.y, PLATFORM_LENGTH };
			if (IsPlayerCollidingWithPlatform(*player, platformData) && game->IsUnevenPlatform == true) {
				if (platforms[i - 1].V2.y >= player->V2.y - 20 && player->V2.x == platforms[i].V2.x + 70) {
					player->V2.y -= 5;
				}
			}
		}
	}

	for (int i = 0; i < MAX_PLATFORMS; i++) {
		if (CheckCollision(player, &platforms[i], &ladders[i], delta)) {
			if (player->leftBorder == true) {
				player->V2.x += 10; player->leftBorder = false;
			}
			else if (player->rightBorder == true) {
				player->V2.x -= 10; player->rightBorder = false;
			}
			else if (player->ItsLadderHere == true && player->CanMoveUp == true) {
				player->V2.y -= 10;
				player->ItsLadderHere = false;
				player->CanMoveUp = false;
			}
			else if (player->ItsLadderHereDown == true && player->CanMoveDown == true) {
				player->V2.y += 10;
				player->ItsLadderHereDown = false;
				player->CanMoveDown = false;
			}
		}
	}
}

void SaveHighScore(const char* pseudonym, int score) {
	FILE* file = fopen("high_scores.txt", "a");
	if (file != NULL) {
		fprintf(file, "%s %d\n", pseudonym, score);
		fclose(file);
		printf("Your score saved!");
	}
}

void DisplayHighScores() {
	FILE* file = fopen("high_scores.txt", "r");
	if (file != NULL) {
		printf("High Scores:\n");
		char pseudonym[20];
		int score;
		while (fscanf(file, "%s %d\n", pseudonym, &score) != EOF) {
			printf("%s: %d\n", pseudonym, score);
		}
		fclose(file);
	}
}

void TextDraw(SDL_Surface* screen, SDL_Surface* charset, Game& game, Player& player, char text[]) {
	sprintf(text, "Zherzdiev Danylo 196765, time = %.1lf s  %.0lf klatek / s", game.worldTime, game.fps);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

	sprintf(text, "Esc - exit, \032 \030 \033 - moving");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

	sprintf(text, "Choose Level: (1) Level 1; (2) Level 2; (3) Level 3;");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);

	sprintf(text, "X: %d | Y: %d", player.V2.x, player.V2.y);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 93, text, charset);

	sprintf(text, "Current Level: %d", game.currentLevel);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 77, text, charset);

	if (game.EndLevel == true || game.EndGame == true) {
		if (game.EndLevel == true) {
			sprintf(text, "Congratulations, you finished this level! You got 100 more points!");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 452, text, charset);
			sprintf(text, "Press - 'L' to play next level!");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 500, text, charset);
		}
		else if (game.EndGame == true) {
			char pseudonym[20];
			if (game.canWrite == true) {
				printf("Game Over!\nEnter your pseudonym: ");
				scanf("%19s", pseudonym);

				SaveHighScore(pseudonym, points->pointsText.count);
				game.canWrite = false;
			}
			sprintf(text, "Congratulations, you finished the game!");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 452, text, charset);
			sprintf(text, "Press - 'W' to save your score and write your nickname!");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 500, text, charset);
		}

		sprintf(text, "Your score: %d", points->pointsText.count);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 468, text, charset);
		sprintf(text, "Press - 'N' to start a new game!");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 516, text, charset);
	}
}

void KEY(SDL_Surface* screen, SDL_Event& event, SDL_Surface* charset, Game& game, Player& player, char text[], int red) {
	while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) game.quit = 1;
				else if (event.key.keysym.sym == SDLK_LEFT) {
					player.isMovingLeft = true;
					player.isMovingRight = false;
					player.speed = 5.0;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT) {
					player.isMovingLeft = false;
					player.isMovingRight = true;
					player.speed = 5.0;
				}
				else if (event.key.keysym.sym == SDLK_UP) {
					player.CanMoveUp = true;
				}
				else if (event.key.keysym.sym == SDLK_DOWN) {
					player.CanMoveDown = true;
				}
				else if (event.key.keysym.sym == SDLK_SPACE && !player.isJumping && !game.EndLevel && !game.EndGame) {
					player.isJumping = true;
					player.jumpStartY = player.V2.y;
				}
				else if (event.key.keysym.sym == SDLK_n) {
					ClearBoard();
					player.V2.x = START_X;
					player.V2.y = START_Y;
					game.worldTime = 0;
					game.Level2 = false;
					game.Level1 = true;
					game.EndLevel = false;
					game.EndGame = false;
					points->hidePoint1 = false;
					points->hidePoint2 = false;
					points->hidePoint3 = false;
					points->pointsText.count = 0;

					InitializePlatforms(screen, player, game, red);
					InitializeLedder(game);
					InitializeSnowman(game);
					InitializePoints(screen, charset, game, player, text);
					printf("New Game has been started!\n\n");
				}
				else if (event.key.keysym.sym == SDLK_1) {
					ClearBoard();
					game.Level1 = true;
					game.Level2 = false;
					game.Level3 = false;
					game.currentLevel = 1;
					InitializePlatforms(screen, player, game, red);
					InitializeLedder(game);
					InitializeSnowman(game);
					InitializePoints(screen, charset, game, player, text);
				}
				else if (event.key.keysym.sym == SDLK_2) {
					ClearBoard();
					game.Level1 = false;
					game.Level2 = true;
					game.Level3 = false;
					game.currentLevel = 2;
					InitializePlatforms(screen, player, game, red);
					InitializeLedder(game);
					InitializeSnowman(game);
					//InitializePoints(screen, charset, game, player, text);
				}
				else if (event.key.keysym.sym == SDLK_3) {
					ClearBoard();
					game.Level1 = false;
					game.Level2 = false;
					game.Level3 = true;
					game.currentLevel = 3;
					InitializePlatforms(screen, player, game, red);
					InitializeLedder(game);
					InitializeSnowman(game);
				}
				else if (event.key.keysym.sym == SDLK_l && (game.Level2 == true || game.Level3 == true)) {
					ClearBoard();
					game.Level1 = false;
					if (game.Level2 == true && game.currentLevel == 1) {
						game.currentLevel = 2;
						game.Level2 = true;
						game.Level3 = false;
					}
					else if (game.Level3 == true && game.currentLevel == 2) {
						game.currentLevel = 3;
						game.Level3 = true;
						game.Level2 = false;
					}
					game.EndLevel = false;
					InitializePlatforms(screen, player, game, red);
					InitializeLedder(game);
					InitializeSnowman(game);
				}
				else if (event.key.keysym.sym == SDLK_w) {
					game.canWrite = true;
				}
				break;
			case SDL_KEYUP:
				player.speed = 0.0;
				player.CanMoveUp = false;
				player.CanMoveDown = false;
				player.isJumping = false;
				break;
			case SDL_QUIT:
				game.quit = 1;
				break;
			};
		};
		game.frames++;
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	Game game;


	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* playerModel;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// fullscreen mode
//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
//	                                 &window, &renderer);
	game.rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (game.rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Danylo Zherzdiev 196765");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	// wy³¹czenie widocznoœci kursora myszy
	//SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	playerModel = SDL_LoadBMP("./player1.bmp");
	if (playerModel == NULL) {
		printf("SDL_LoadBMP(playerModel.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	char text[128];
	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	game.timer1 = SDL_GetTicks();

	Player player;
	player.V2.x = START_X;
	player.V2.y = START_Y;
	player.width = playerModel->w;
	player.height = playerModel->h;
	player.speed = 0.0;
	player.isJumping = false;
	player.jumpStartY = 0.0;
	player.jumpVelocity = 0.0;

	InitializePlatforms(screen, player, game, red);
	InitializeLedder(game);
	InitializeSnowman(game);

	while (!game.quit) {
		//printf("%d", game.IsUnevenPlatform);
		//printf("%d", player.isOnLadder);

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

		UpdatePlayer(screen, &player, points, &game, game.delta);

		SDL_FillRect(screen, NULL, black);

		DrawPlatforms(screen, game, player, game.delta);
		DrawLedder(screen);
		DrawSnowman(screen);

		DrawSurface(screen, playerModel, player.V2.x, player.V2.y);

		// info text
		DrawRectangle(screen, 0, 4, SCREEN_WIDTH, 70, red, blue);
		//DrawRectangle(screen, screen->w / 2 - strlen(text) * 8, 74, 245, 30, red, 0);

		TextDraw(screen, charset, game, player, text);
		InitializePoints(screen, charset, game, player, text);

		DrawPoints(screen);

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		KEY(screen, event, charset, game, player, text, red);
	}
	// free all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};
