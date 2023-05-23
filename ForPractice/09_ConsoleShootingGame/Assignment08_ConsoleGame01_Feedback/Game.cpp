#include "Game.h"
#include "FileIO.h"
#include "Scenes.h"
#include <Windows.h>
#include <time.h>

enum GAMESTATE
{
	STATE_PLAY,
	STATE_CLEAR,
	STATE_OVER
};

int gameStageFlag = STATE_PLAY;
char strStage[INFO_SIZE];
Player player;
clock_t moveStart = clock();

// About Enemy ==========================================
char posData[DATA_SIZE];

int enemyDataCnt = 0;
EnemyData* enemyData;

// About bullet ==========================================

Bullet bullets[BULLET_CNT];
int bulletMax = 0;
int bulletCur = 0;
clock_t bulletStart = clock();

// Function ==============================================

void UpdateGame()
{
	switch (gameStageFlag)
	{
	case STATE_PLAY:

		// input section
		GetKeyGame();

		// logic section
		UpdatePlayer();
		UpdateBullet();
		UpdateEnemy();

		// render section
		rd_BufferClear();
		GameDataToBuffer();
		rd_BufferFlip();

		break;

	case STATE_CLEAR:
		GameClear();
		break;

	case STATE_OVER:
		GameOver();
		break;
	}
}

// Get Input =================================================

void GetKeyGame()
{
	clock_t moveEnd = clock();
	float time = (float)(moveEnd - moveStart) / CLOCKS_PER_SEC;

	if (GetAsyncKeyState(VK_ESCAPE))
	{
		g_exit = true;
	}
	else if (GetAsyncKeyState(VK_LEFT))
	{
		if (player._iX > 0 && time > player._fSpeed)
		{
			player._iX--;
			moveStart = clock();
		}
			
	}
	else if (GetAsyncKeyState(VK_RIGHT))
	{
		if (player._iX < dfSCREEN_WIDTH - 3
			&& time > player._fSpeed)
		{
			player._iX++;
			moveStart = clock();
		}		
	}
}


// Update =================================================
void UpdateBullet()
{
	float time = (float)(clock() - bulletStart) / CLOCKS_PER_SEC;

	if (time < bullets[bulletCur]._iSpeed)
		return;

	if (bulletMax < BULLET_CNT)
		bulletMax++;

	bullets[bulletCur]._iX = player._iX;
	bullets[bulletCur]._iY = player._iY;
	bullets[bulletCur]._bDead = false;

	for (int i = 0; i < bulletMax; i++)
	{
		bullets[i]._iY--;
	}

	bulletCur++;

	if (bulletCur >= BULLET_CNT)
		bulletCur = 0;

	bulletStart = clock();
}

void UpdateEnemy()
{
	int enemyCnt = 0;

	for (int i = 0; i < enemyDataCnt; i++)
	{
		EnemyData* tmp = &enemyData[i];

		for (int j = 0; j < tmp->_iMax; j++)
		{
			Enemy* enemy = &(tmp->_enemies)[j];

			for (int k = 0; k < bulletMax; k++)
			{
				if (enemy->_iX == bullets[k]._iX
					&& enemy->_iY == bullets[k]._iY
					&& bullets[k]._bDead == false
					&& enemy->_bDead == false)
				{
					enemy->_iHp -= bullets[k]._iAttack;
					bullets[k]._bDead = true;
				}
			}

			if (enemy->_iHp <= 0)
				enemy->_bDead = true;
			else
				enemyCnt++;
		}
	}

	if (enemyCnt <= 0)
	{
		gameStageFlag = STATE_CLEAR;
		return;
	}

}

void UpdatePlayer()
{
	if (player._iHp <= 0)
	{
		gameStageFlag = STATE_OVER;
		return;
	}
}

// Render =================================================

void GameDataToBuffer()
{
	for (int i = 0; i < bulletMax; i++)
	{
		if (bullets[i]._bDead == false)
			rd_SpriteDraw(bullets[i]._iX, bullets[i]._iY, bullets[i]._chIcon[0]);
	}

	Enemy* enemy;
	EnemyData* tmp;

	for (int i = 0; i < enemyDataCnt; i++)
	{
		tmp = &enemyData[i];

		for (int j = 0; j < tmp->_iMax; j++)
		{
			enemy = &(tmp->_enemies)[j];

			if (enemy->_bDead == false)
				rd_SpriteDraw(enemy->_iX, enemy->_iY, *tmp->_chIcon);
		}
	}

	rd_SpriteDraw(player._iX, player._iY, player._chIcon[0]);

	for (int i = 0; i < strlen(strStage); i++)
	{
		rd_SpriteDraw(i, 0, strStage[i]);
	}
}

// Change Scene =============================================

void FreeHeapData()
{
	for (int i = 0; i < enemyDataCnt; i++)
	{
		free(enemyData[i]._enemies);
	}
	free(enemyData);
}

void GameClear()
{
	FreeHeapData();
	gameStageFlag = STATE_PLAY;
	g_Scene = CLEAR;
}

void GameOver()
{
	FreeHeapData();
	gameStageFlag = STATE_PLAY;
	g_Scene = OVER;
}