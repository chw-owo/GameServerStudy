#pragma once
#include "windows.h"
#define GRID_WIDTH 100
#define GRID_HEIGHT 50

class CMap
{
public:
	CMap();
	~CMap();

public:
	void Draw(int xPos, int yPos);
	void DrawRandom();
	void FindPath();

public:
	void RenderGrid(HDC hdc);
	void RenderColor(HDC hdc);

public:
	void MoveRight() { _iXPad -= 3; }
	void MoveLeft() { _iXPad += 3; }
	void MoveUp() { _iYPad += 3; }
	void MoveDown() { _iYPad -= 3; }

	void SetDraw(bool bDraw) { _bDraw = bDraw; }
	void SetErase(bool bErase) { _bErase = bErase; }
	void SelectStart(bool bSelectStart) { _bSelectStart = bSelectStart; }
	void SelectDest(bool bSelectDest) { _bSelectDest = bSelectDest; }

	void SetScale(short wParam)
	{
		if (wParam > 0)
			_iGridSize++;
		else if (wParam < 0)
			_iGridSize--;

		if (_iGridSize == 0) _iGridSize = 1;
	}
	
private:
	enum MAP
	{
		NONE = 0,
		OBSTACLE,
		START,
		DEST
	};

	int _iXPad = 0;
	int _iYPad = 0;
	int _iGridSize = 16;
	char _chMap[GRID_HEIGHT][GRID_WIDTH] = { NONE, };

private:
	HPEN _hGridPen;
	HBRUSH _hTileBrush;
	HBRUSH _hStartBrush;
	HBRUSH _hDestBrush;

private:
	bool _bDraw = false;
	bool _bErase = false;
	bool _bSelectStart = false;
	bool _bSelectDest = false;

private:
	struct Pos
	{
		int _X = 0;
		int _Y = 0;
	};

	Pos _pStartPos;
	Pos _pDestPos;
};

