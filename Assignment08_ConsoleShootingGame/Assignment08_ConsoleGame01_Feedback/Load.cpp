#include "Load.h"
#include "Render.h"
#include "FileIO.h"
#include "Game.h"

#include <Windows.h>
#include <iostream>

void UpdateLoad()
{
	// input section
	//GetKeyLoad();

	// logic section
	LoadGameData();

	// render section
	// rd_BufferClear();
	// rd_DataToBuffer();
	// rd_BufferFlip();
}

void GetKeyLoad()
{

}

void LoadGameData()
{
	int resultSize;

	// Load Current Stage Data by StageInfo.txt
	char stageDataRoot[ROOT_LEN];
	LoadTokenedData(sceneFileRoot[GAME], stageDataRoot,
		resultSize, GET_ROOT_ONE, g_Stage);

	// Load Data Root by Stage.txt
	char gameDataRoot[DATA_END][ROOT_CNT][ROOT_LEN];
	LoadTokenedData(stageDataRoot, gameDataRoot,
		resultSize, GET_GAMEDATA);
	enemyDataCnt = resultSize;

	// Load Pos Data
	LoadOriginData(gameDataRoot[DATA_POS][0], posData, resultSize);

	// Load Player Data
	LoadTokenedData(gameDataRoot[DATA_PLAYER][0], &player,
		resultSize, GET_DATA_PLAYER);

	// Load Bullet Data
	LoadTokenedData(gameDataRoot[DATA_BULLET][0], bullets,
		resultSize, GET_DATA_BULLET);

	// Load Enemy Data
	int dataSize = enemyDataCnt * sizeof(EnemyData);
	enemyData = (EnemyData*)(malloc(dataSize));
	memset(enemyData, 0, dataSize);

	for (int i = 0; i < enemyDataCnt; i++)
	{
		LoadTokenedData(gameDataRoot[DATA_ENEMY][i], &enemyData[i],
			resultSize, GET_DATA_ENEMY);

		enemyData[i]._iMax = atoi(gameDataRoot[DATA_ENEMY_NUM][i]);
	}

	InitialEnemy();
	ConvertStageToString();

	g_Scene = GAME;
}

void InitialEnemy()
{
	int iX = 0;
	int iY = 0;
	int enemyMax = 0;

	for (int i = 0; i < enemyDataCnt; i++)
	{
		enemyMax = enemyData[i]._iMax;
		enemyData[i]._enemies
			= (Enemy*)(malloc(sizeof(Enemy) * enemyMax));
	}

	EnemyData* tmp;
	Enemy* enemy;

	for (int i = 0; i < DATA_SIZE; i++)
	{
		if (posData[i] == ' ')
		{
			iX++;
		}
		else if (posData[i] == '\n')
		{
			iY++;
			iX = 0;
		}
		else
		{
			iX++;

			for (int j = 0; j < enemyDataCnt; j++)
			{
				if (posData[i] == *(enemyData[j]._chName))
				{
					tmp = &enemyData[j];
					enemy = (tmp->_enemies) + (tmp->_iCnt);

					enemy->_bDead = false;
					enemy->_iX = iX;
					enemy->_iY = iY;
					enemy->_iHp = tmp->_iTotalHp;

					tmp->_iCnt++;
				}
			}
		}
	}
}

void ConvertStageToString()
{
	sprintf_s(strStage, INFO_SIZE, "Stage: %d", g_Stage + 1);
}