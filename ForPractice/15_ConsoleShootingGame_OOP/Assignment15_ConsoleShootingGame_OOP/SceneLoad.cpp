#include "SceneLoad.h"

#include "ObjectManager.h"
#include "ScreenBuffer.h"
#include "PlayerObject.h"
#include "BulletObject.h"
#include "EnemyObject.h"

#include "string.h"
#include <Windows.h>
#include <iostream>

#define POS_DATA_SIZE 1024

CSceneLoad::CSceneLoad()
{
	_SceneType = LOAD;
	_pObjectMgr = CObjectManager::GetInstance();
	_pScreenBuffer = CScreenBuffer::GetInstance();
	GetGameData();
}

void CSceneLoad::Update()
{
	// input section
	GetKeyInput();

	// login section
	//_pObjectMgr->Update();

	// render section
	_pScreenBuffer->BufferClear();
	//_pObjectMgr->Render();
	_pScreenBuffer->BufferFlip();
}

void CSceneLoad::GetKeyInput()
{
	// Exit
	if (GetAsyncKeyState(VK_ESCAPE))
	{
		_bExit = true;
	}
}


void CSceneLoad::GetGameData()
{
	int size = 0;

	// Get Current Stage =============================================================

	FILE* pFile;
	fopen_s(&pFile, ".\\Data\\PlayData.txt", "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* playData = (char*)(malloc(size));
	fread(playData, 1, size, pFile);
	fclose(pFile);
	_iPlayData = atoi(playData);
	free(playData);

	// Get Final Stage ===============================================================

	fopen_s(&pFile, ".\\Data\\PlayFinalData.txt", "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* playFinalData = (char*)(malloc(size));
	fread(playFinalData, 1, size, pFile);
	fclose(pFile);
	_iPlayFinalData = atoi(playFinalData);
	free(playFinalData);

	// Get Current Stage Info Root from StageInfo.txt =================================

	fopen_s(&pFile, ".\\Data\\Game\\StageInfo.txt", "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* stageInfo = (char*)(malloc(size));
	fread(stageInfo, 1, size, pFile);
	fclose(pFile);

	char sep[] = { "\n " };
	char* tok = NULL;
	char* next = NULL;

	int cnt = 0;
	char* curStageRoot = (char*)(malloc(size));
	tok = strtok_s(stageInfo, sep, &next);

	while (cnt < _iPlayData && tok != NULL)
	{
		tok = strtok_s(NULL, sep, &next);
		cnt++;
	}

	strcpy_s(curStageRoot, size, tok);
	free(stageInfo);

	// Get Current Stage Info ============================================================

	fopen_s(&pFile, curStageRoot, "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* curStageInfo = (char*)(malloc(size));
	fread(curStageInfo, 1, size, pFile);
	fclose(pFile);
	free(curStageRoot);

	// Get Game Data Root ================================================================

	tok = NULL;
	next = NULL;
	tok = strtok_s(curStageInfo, sep, &next);

	//Set PLAYER =========================================================================

	tok = strtok_s(NULL, sep, &next);
	fopen_s(&pFile, tok, "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* playerData = (char*)(malloc(size));
	fread(playerData, 1, size, pFile);
	fclose(pFile);

	char playerIcon;
	int playerHp;
	int playerSpeed;

	char playerSep[] = { "\n" };
	char* playerTok = NULL;
	char* playerNext = NULL;

	playerTok = strtok_s(playerData, playerSep, &playerNext);
	playerIcon = playerTok[0];
	playerTok = strtok_s(NULL, playerSep, &playerNext);
	playerHp = atoi(playerTok);
	playerTok = strtok_s(NULL, playerSep, &playerNext);
	playerSpeed = atoi(playerTok);
	free(playerData);

	CPlayerObject player;
	CPlayerObject* pPlayer = _pObjectMgr->CreateObject(
		&player, playerIcon, playerHp, playerSpeed);

	//Set BULLET =========================================================================

	tok = strtok_s(NULL, sep, &next);
	tok = strtok_s(NULL, sep, &next);

	fopen_s(&pFile, tok, "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* bulletData = (char*)(malloc(size));
	fread(bulletData, 1, size, pFile);
	fclose(pFile);

	char bulletIcon;
	int bulletAttack;
	int bulletSpeed;

	char bulletSep[] = { "\n" };
	char* bulletTok = NULL;
	char* bulletNext = NULL;

	bulletTok = strtok_s(bulletData, bulletSep, &bulletNext);
	int _iBulletMax = atoi(bulletTok);
	bulletTok = strtok_s(NULL, bulletSep, &bulletNext);
	bulletIcon = bulletTok[0];
	bulletTok = strtok_s(NULL, bulletSep, &bulletNext);
	bulletAttack = atoi(bulletTok);
	bulletTok = strtok_s(NULL, bulletSep, &bulletNext);
	bulletSpeed = atoi(bulletTok);
	free(bulletData);

	CBulletObject bullet;
	for (int i = 0; i < _iBulletMax; i++)
	{
		_pObjectMgr->CreateObject(
			&bullet, bulletIcon, bulletAttack, bulletSpeed, pPlayer);
	}

	// Set ENEMY_POSITION ================================================================

	tok = strtok_s(NULL, sep, &next);
	tok = strtok_s(NULL, sep, &next);

	char enemyPosition[POS_DATA_SIZE];
	fopen_s(&pFile, tok, "r");
	fread(enemyPosition, 1, POS_DATA_SIZE, pFile);
	fclose(pFile);

	// Set ENEMY =========================================================================	

	tok = strtok_s(NULL, sep, &next);
	tok = strtok_s(NULL, sep, &next);

	int enemyNum = atoi(tok);

	for (int i = 0; i < enemyNum; i++)
	{
		tok = strtok_s(NULL, sep, &next);
		char* root = tok;
		tok = strtok_s(NULL, sep, &next);
		int num = atoi(tok);

		FILE* pFile;
		fopen_s(&pFile, root, "r");
		fseek(pFile, 0, SEEK_END);
		size = ftell(pFile);
		rewind(pFile);
		char* enemyData = (char*)(malloc(size));
		fread(enemyData, 1, size, pFile);
		fclose(pFile);

		char enemyType;
		char enemyIcon;
		int enemyHp;

		char enemySep[] = { "\n" };
		char* enemyTok = NULL;
		char* enemyNext = NULL;

		enemyTok = strtok_s(enemyData, enemySep, &enemyNext);
		enemyType = enemyTok[0];
		enemyTok = strtok_s(NULL, enemySep, &enemyNext);
		enemyIcon = enemyTok[0];
		enemyTok = strtok_s(NULL, enemySep, &enemyNext);
		enemyHp = atoi(enemyTok);
		free(enemyData);

		int idx = 0;
		int x = 0;
		int y = 0;
		CEnemyObject enemy;

		for (int i = 0; i < num; i++)
		{
			for (; idx < POS_DATA_SIZE; idx++)
			{
				if (enemyPosition[idx] == enemyType)
				{
					_pObjectMgr->CreateObject(
						&enemy, x, y, enemyType, enemyIcon, enemyHp);
					idx++;
					break;
				}
				else if (enemyPosition[idx] == ' ')
				{
					x++;
				}
				else if (enemyPosition[idx] == '\n')
				{
					y++;
					x = 0;
				}
			}
		}
	}

	free(curStageInfo);

	_SceneType = GAME;
}

CSceneLoad::~CSceneLoad()
{

}