#include "Title.h"
#include "FileIO.h"
#include "Render.h"
#include <Windows.h>

bool titleInitFlag = false;
char titleData[DATA_SIZE];

void UpdateTitle()
{
	// input section
	GetKeyTitle();

	// logic section
	if (titleInitFlag == false)
	{
		LoadTitleData();
		titleInitFlag = true;
	}

	// render section
	rd_BufferClear();
	rd_DataToBuffer(titleData);
	rd_BufferFlip();
}

void GetKeyTitle()
{
	// Continue Prev Play
	if (GetAsyncKeyState(VK_NUMPAD1)
		|| GetAsyncKeyState(0x31))
	{
		g_Scene = LOAD;
	}
	// Start New Play
	else if (GetAsyncKeyState(VK_NUMPAD2)
		|| GetAsyncKeyState(0x32))
	{
		SetStageData(0);
		g_Scene = LOAD;
	}
	// Exit
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		g_exit = true;
	}
}


void LoadTitleData()
{
	int tmpSize;
	LoadOriginData(sceneFileRoot[TITLE], titleData, tmpSize);
}
