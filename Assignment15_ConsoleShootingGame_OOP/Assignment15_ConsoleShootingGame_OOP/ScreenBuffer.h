#pragma once
#include "Windows.h"
#define dfSCREEN_HEIGHT 24
#define dfSCREEN_WIDTH 81

class CScreenBuffer
{
public:
	static CScreenBuffer* GetInstance(void);
	void BufferFlip(void);
	void BufferClear(void);
	void SpriteDraw(int iX, int iY, char chSprite);
	
private:
	CScreenBuffer();
	~CScreenBuffer();
	void MoveCursor(int iPosX, int iPosY);

private:
	static CScreenBuffer _ScreenBuffer;
	char _cScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];
	HANDLE _hConsole;
};
