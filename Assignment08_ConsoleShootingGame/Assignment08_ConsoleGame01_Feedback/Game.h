#pragma once
#include "Render.h"

#define BULLET_CNT 32

#define ICON_SIZE 8
#define NAME_SIZE 8
#define INFO_SIZE 16

enum GAMEDATA
{
	DATA_POS,
	DATA_PLAYER,
	DATA_BULLET,
	DATA_ENEMY,
	DATA_ENEMY_NUM,
	DATA_END
};

struct Player
{
	bool _bDead = false;
	char _chIcon[ICON_SIZE] = { '0', };
	int _iX = dfSCREEN_WIDTH / 2;
	int _iY = (dfSCREEN_HEIGHT / 4) * 3;
	int _iTotalHp = 0;
	int _iHp = 0;
	float _fSpeed = 0;
};

struct Bullet
{
	bool _bDead = false;
	char _chIcon[ICON_SIZE] = { '0', };
	int _iX = -1;
	int _iY = -1;
	int _iAttack = 0;
	float _iSpeed = 0;

};

struct Enemy
{
	bool _bDead = false;
	int _iX = 0;
	int _iY = 0;
	int _iHp = 0;
};

struct EnemyData
{
	char _chName[NAME_SIZE] = { '0', };
	char _chIcon[ICON_SIZE] = { '0', };
	Enemy* _enemies;
	int _iTotalHp = 0;
	int _iMax = 0;
	int _iCnt = 0;
};

extern Player player;
extern Bullet bullets[BULLET_CNT];

extern char posData[DATA_SIZE];
extern EnemyData* enemyData;
extern int enemyDataCnt;

extern char strStage[INFO_SIZE];

// Main
void UpdateGame();

// Get Input
void GetKeyGame();

// Update
void UpdateBullet();
void UpdateEnemy();
void UpdatePlayer();

// Render
void GameDataToBuffer();

// Scene Change
void FreeHeapData();
void GameClear();
void GameOver();
