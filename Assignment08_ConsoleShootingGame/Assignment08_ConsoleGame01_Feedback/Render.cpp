#include "Render.h"
#include <stdio.h>
#include <memory.h>

char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];

void rd_BufferFlip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}

void rd_BufferClear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}
}

void rd_SpriteDraw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHT)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}

void rd_DataToBuffer(const char* data)
{
	int iX = 0;
	int iY = 0;

	for (int i = 0; i < DATA_SIZE; i++)
	{
		if (data[i] == ' ')
		{
			iX++;
		}
		else if (data[i] == '\n')
		{
			iY++;
			iX = 0;
		}
		else
		{
			iX++;
			rd_SpriteDraw(iX, iY, data[i]);
		}
	}
}


