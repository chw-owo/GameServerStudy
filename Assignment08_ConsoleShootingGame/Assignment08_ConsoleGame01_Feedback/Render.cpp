#include "Render.h"
#include <stdio.h>
#include <memory.h>

char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];

// 버퍼의 내용을 화면으로 찍어주는 함수.
// 적군,아군,총알 등을 szScreenBuffer 에 넣어주고, 
// 한 프레임이 끝나는 마지막에 본 함수를 호출하여 버퍼 -> 화면 으로 그린다.
void rd_BufferFlip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}

// 화면 버퍼를 지워주는 함수
// 매 프레임 그림을 그리기 직전에 버퍼를 지워 준다. 
void rd_BufferClear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}
}

// 버퍼의 특정 위치에 원하는 문자를 출력.
// 입력 받은 X,Y 좌표에 아스키코드 하나를 출력한다. (버퍼에 그림)
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


