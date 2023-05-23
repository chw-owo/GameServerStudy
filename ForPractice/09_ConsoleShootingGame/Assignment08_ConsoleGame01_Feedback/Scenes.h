#pragma once

#define ROOT_LEN 128
enum SCENE
{
	TITLE,
	LOAD,
	GAME,
	CLEAR,
	OVER,
	END
};

extern bool g_exit;
extern int g_Scene;
extern int g_Stage;
extern int g_finalStage;

extern char rootData[];
extern char playData[];
extern char playFinalData[];
extern char sceneFileRoot[END][ROOT_LEN];

void SetExitTrue(char func[], int line);

void InitialSceneData();
void InitialStageData();
