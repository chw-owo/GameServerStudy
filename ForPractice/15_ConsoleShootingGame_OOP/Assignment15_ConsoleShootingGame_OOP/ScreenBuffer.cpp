#include "ScreenBuffer.h"
#include <windows.h>
#include <iostream>

CScreenBuffer::CScreenBuffer()
{
	CONSOLE_CURSOR_INFO stConsoleCursor;

	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1;

	_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(_hConsole, &stConsoleCursor);
}

CScreenBuffer::~CScreenBuffer()
{

}

CScreenBuffer* CScreenBuffer::GetInstance(void)
{
	static CScreenBuffer _ScreenBuffer;
	return &_ScreenBuffer;
}

void CScreenBuffer::BufferClear(void)
{
	int iCnt;
	memset(_cScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		_cScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}
}

void CScreenBuffer::BufferFlip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		MoveCursor(0, iCnt);
		printf(_cScreenBuffer[iCnt]);
	}
}

void CScreenBuffer::SpriteDraw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHT)
		return;

	_cScreenBuffer[iY][iX] = chSprite;
}


void CScreenBuffer::MoveCursor(int iPosX, int iPosY)
{
	COORD stCoord;
	stCoord.X = iPosX;
	stCoord.Y = iPosY;

	SetConsoleCursorPosition(_hConsole, stCoord);
}
