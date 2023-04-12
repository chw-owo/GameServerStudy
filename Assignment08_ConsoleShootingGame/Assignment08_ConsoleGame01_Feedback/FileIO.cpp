#include "FileIO.h"
#include <iostream>

void SetStageData(int stage)
{
	g_Stage = stage;

	FILE* pFile;
	char stageData[NUM_SIZE] = { 0, };
	sprintf_s(stageData, NUM_SIZE, "%d", g_Stage);

	fopen_s(&pFile, playData, "w");
	fwrite(stageData, 1, 1, pFile);
	fclose(pFile);
}

void LoadOriginData(const char* fileName, void* output, int& resultSize)
{
	FILE* pFile;

	fopen_s(&pFile, fileName, "r");
	fseek(pFile, 0, SEEK_END);
	resultSize = ftell(pFile);
	rewind(pFile);
	fread((char*)(output), 1, resultSize, pFile);
	fclose(pFile);
}

void LoadTokenedData(const char* fileName, void* output, int& resultSize, LOAD_FLAG flag, int idx)
{
	FILE* pFile;
	char* fileData;

	fopen_s(&pFile, fileName, "r");
	fseek(pFile, 0, SEEK_END);
	resultSize = ftell(pFile);
	rewind(pFile);

	fileData = (char*)(malloc(resultSize));
	memset(fileData, 0, sizeof(char));
	fread(fileData, 1, resultSize, pFile);
	fclose(pFile);

	char sep[] = { "\n" };

	switch (flag)
	{
	case GET_ROOT_ALL:
		TokenizeRootAll(fileData, sep, resultSize,
			(char(*)[ROOT_LEN])(output));
		break;

	case GET_ROOT_ONE:
		TokenizeRootOne(fileData, sep, idx,
			(char*)(output));
		break;

	case GET_GAMEDATA:
		TokenizeGameData(fileData, sep, resultSize,
			(char(*)[ROOT_CNT][ROOT_LEN])(output));
		break;

	case GET_DATA_ENEMY:
		TokenizeEnemyData(fileData, sep, (EnemyData*)(output));
		break;

	case GET_DATA_PLAYER:
		TokenizePlayerData(fileData, sep, (Player*)(output));
		break;

	case GET_DATA_BULLET:
		TokenizeBulletData(fileData, sep, (Bullet*)(output));
		break;


	default:
		break;
	}

	free(fileData);
}

void TokenizeGameData(char* fileData, const char* sep, int& fileNum, char output[][ROOT_CNT][ROOT_LEN])
{
	int idx1 = 0;
	int idx2 = 0;
	char* tok = NULL;
	char* next = NULL;

	tok = strtok_s(fileData, sep, &next);
	while (tok != NULL)
	{
		if (strcmp(tok, "POSITION") == 0)
		{
			idx1 = DATA_POS;
			idx2 = 0;
		}
		else if (strcmp(tok, "PLAYER") == 0)
		{
			idx1 = DATA_PLAYER;
			idx2 = 0;
		}
		else if (strcmp(tok, "BULLET") == 0)
		{
			idx1 = DATA_BULLET;
			idx2 = 0;
		}
		else if (strcmp(tok, "ENEMY") == 0)
		{
			idx1 = DATA_ENEMY;
			idx2 = 0;
		}
		else if (strcmp(tok, "ENEMYNUM") == 0)
		{
			idx1 = DATA_ENEMY_NUM;
			idx2 = 0;
		}
		else
		{
			strcpy_s(output[idx1][idx2], ROOT_LEN, tok);
			idx2++;
		}

		tok = strtok_s(NULL, sep, &next);
	}

	fileNum = idx2;
}

void TokenizeRootOne(char* fileData, const char* sep, int idx, char output[])
{
	int cnt = 0;
	char* tok = NULL;
	char* next = NULL;

	tok = strtok_s(fileData, sep, &next);
	while (cnt < idx && tok != NULL)
	{
		tok = strtok_s(NULL, sep, &next);
		cnt++;
	}
	strcpy_s(output, ROOT_LEN, tok);
}

void TokenizeRootAll(char* fileData, const char* sep, int& fileNum, char output[][ROOT_LEN])
{
	int idx = 0;
	char* tok = NULL;
	char* next = NULL;

	tok = strtok_s(fileData, sep, &next);
	while (tok != NULL)
	{
		strcpy_s(output[idx], ROOT_LEN, tok);
		tok = strtok_s(NULL, sep, &next);
		idx++;
	}
	fileNum = idx;
}


void TokenizeEnemyData(char* fileData, const char* sep, EnemyData* output)
{
	int idx = 0;
	char* tok = NULL;
	char* next = NULL;

	tok = strtok_s(fileData, sep, &next);
	strcpy_s(output->_chName, NAME_SIZE, tok);

	tok = strtok_s(NULL, sep, &next);
	strcpy_s(output->_chIcon, ICON_SIZE, tok);

	tok = strtok_s(NULL, sep, &next);
	output->_iTotalHp = atoi(tok);
	output->_iCnt = 0;
}

void TokenizePlayerData(char* fileData, const char* sep, Player* output)
{
	int idx = 0;
	char* tok = NULL;
	char* next = NULL;

	tok = strtok_s(fileData, sep, &next);
	strcpy_s(output->_chIcon, ICON_SIZE, tok);

	tok = strtok_s(NULL, sep, &next);
	output->_iTotalHp = atoi(tok);
	output->_iHp = atoi(tok);

	tok = strtok_s(NULL, sep, &next);
	output->_fSpeed = (float)1 / atoi(tok);
}

void TokenizeBulletData(char* fileData, const char* sep, Bullet* output)
{
	int idx = 0;
	char* tok = NULL;
	char* next = NULL;
	tok = strtok_s(fileData, sep, &next);

	for (int i = 0; i < BULLET_CNT; i++)
		strcpy_s(output[i]._chIcon, ICON_SIZE, tok);

	tok = strtok_s(NULL, sep, &next);
	for (int i = 0; i < BULLET_CNT; i++)
		output[i]._iAttack = atoi(tok);

	tok = strtok_s(NULL, sep, &next);
	for (int i = 0; i < BULLET_CNT; i++)
		output[i]._iSpeed = atof(tok);
}