#pragma once
#include "Game.h"
#include "Scenes.h"

#define ROOT_CNT 16
#define NUM_SIZE 8

enum LOAD_FLAG {

	GET_ROOT_ALL,
	GET_ROOT_ONE,
	GET_GAMEDATA,
	GET_DATA_ENEMY,
	GET_DATA_PLAYER,
	GET_DATA_BULLET
};

void SetStageData(int stage);
void LoadOriginData(const char* fileName, void* output, int& resultSize);
void LoadTokenedData(const char* fileName, void* output, int& resultSize, LOAD_FLAG flag, int idx = 0);

void TokenizeRootAll(char* fileData, const char* sep, int& fileNum, char output[][ROOT_LEN]);
void TokenizeRootOne(char* fileData, const char* sep, int idx, char output[]);
void TokenizeGameData(char* fileData, const char* sep, int& fileNum, char output[][ROOT_CNT][ROOT_LEN]);

void TokenizeEnemyData(char* fileData, const char* sep, EnemyData* output);
void TokenizePlayerData(char* fileData, const char* sep, Player* output);
void TokenizeBulletData(char* fileData, const char* sep, Bullet* output);