#pragma once
#include "windows.h"
#include "Astar.h"

class CPathFindingTool
{
public:
	CPathFindingTool();
	~CPathFindingTool();

public:
	void StartFindPath();
	void FindPath();

public:
	void Draw(int xPos, int yPos);
	void DrawRandom();
	void Render(HDC hdc);

private:
	void RenderGrid(HDC hdc);
	void RenderColor(HDC hdc);
	void RenderPath(HDC hdc);
	void RenderPathFinderData(HDC hdc);

public:
	void MoveRight() { _iXPad -= 3; }
	void MoveLeft() { _iXPad += 3; }
	void MoveUp() { _iYPad += 3; }
	void MoveDown() { _iYPad -= 3; }

	void SetScale(short wParam);
	void SetDraw(bool bDraw) { _bDraw = bDraw; }
	void SetErase(bool bErase) { _bErase = bErase; }
	void SelectStart(bool bSelectStart) { _bSelectStart = bSelectStart; }
	void SelectDest(bool bSelectDest) { _bSelectDest = bSelectDest; }
	
public:
	bool _bFindPath = false;

private:
	int _iXPad = 0;
	int _iYPad = 0;
	int _iGridSize = 16;

private:
	HPEN _hGridPen;
	HBRUSH _hObstacleBrush;
	HBRUSH _hOpenBrush;
	HBRUSH _hCloseBrush;
	HBRUSH _hStartBrush;
	HBRUSH _hDestBrush;

private:
	bool _bDraw = false;
	bool _bErase = false;
	bool _bSelectStart = false;
	bool _bSelectDest = false;

private:
	Pos _pStartPos;
	Pos _pDestPos;
	PathFinder* _pathFinder;
};

