#include "Scenes.h"
#include "FileIO.h"
#include <iostream>

bool g_exit = false;
int g_Scene = 0;
int g_Stage = 0;
int g_finalStage;

char rootData[] = ".\\Data\\RootData.txt";
char playData[] = ".\\Data\\PlayData.txt";
char playFinalData[] = ".\\Data\\PlayFinalData.txt";
char sceneFileRoot[END][ROOT_LEN] = { '\0', };

void InitialSceneData()
{
	int tmpSize;
	LoadTokenedData(rootData, sceneFileRoot, tmpSize, GET_ROOT_ALL);
}

void InitialStageData()
{
	int tmpSize;

	char curStage[NUM_SIZE] = { '\0', };
	LoadOriginData(playData, curStage, tmpSize);
	g_Stage = atoi(curStage);

	char finalStageTmp[NUM_SIZE] = { '\0', };
	LoadOriginData(playFinalData, finalStageTmp, tmpSize);
	g_finalStage = atoi(finalStageTmp);
}

void SetExitTrue(char func[], int line)
{
	printf("%s, line %d: \n", func, line);
	g_exit = true;
}


