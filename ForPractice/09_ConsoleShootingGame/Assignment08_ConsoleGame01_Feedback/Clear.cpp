#include "Clear.h"
#include "Scenes.h"
#include "Render.h"
#include "FileIO.h"
#include <Windows.h>
#include <iostream>

bool clearInitFlag = false;
char clearData[DATA_SIZE];

void UpdateClear()
{
	// input section
	GetKeyClear();

	// logic section
	if (clearInitFlag == false)
	{
		InitialClear();
		clearInitFlag = true;
	}

	// render section
	rd_BufferClear();
	rd_DataToBuffer(clearData);
	rd_BufferFlip();
}

void GetKeyClear()
{
	if (GetAsyncKeyState(VK_RETURN))
	{
		if (g_Stage >= g_finalStage)
		{
			clearInitFlag = false;
			g_Scene = TITLE;
		}
		else
		{
			clearInitFlag = false;
			SetStageData(g_Stage + 1);
			g_Scene = LOAD;
		}
	}
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		clearInitFlag = false;
		g_exit = true;
	}
}

void InitialClear()
{
	int tmpSize;
	char clearDataRoot[ROOT_LEN];
	LoadTokenedData(sceneFileRoot[CLEAR], clearDataRoot,
		tmpSize, GET_ROOT_ONE, g_Stage);

	memset(clearData, ' ', DATA_SIZE);
	LoadOriginData(clearDataRoot, clearData, tmpSize);

}
