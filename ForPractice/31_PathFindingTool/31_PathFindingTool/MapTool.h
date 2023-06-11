#pragma once
#include <windows.h>
#include "Map.h"

#define MOVESPEED 10
#define DEFAULT_X_PAD 100
#define DEFAULT_Y_PAD 60
#define DEFAULT_GRID_SIZE 8

class MapTool
{
public:
	MapTool();
	~MapTool();

public:
	void Draw(int xPos, int yPos);
	void DrawRandom();
	void Render(HDC hdc);

private:
	void RenderMenu(HDC hdc);
	void RenderGrid(HDC hdc);
	void RenderColor(HDC hdc);
	void RenderPath(HDC hdc);
	void RenderPathFinderData(HDC hdc);

public:
	void MoveRight() { _iXPad -= MOVESPEED; }
	void MoveLeft() { _iXPad += MOVESPEED; }
	void MoveUp() { _iYPad += MOVESPEED; }
	void MoveDown() { _iYPad -= MOVESPEED; }

	void SetScale(short wParam);
	void SetDraw(bool bDraw) { _bDraw = bDraw; }
	void SetErase(bool bErase) { _bErase = bErase; }
	void SelectStart(bool bSelectStart) { _bSelectStart = bSelectStart; }
	void SelectDest(bool bSelectDest) { _bSelectDest = bSelectDest; }

private:
	int _iXPad = DEFAULT_X_PAD;
	int _iYPad = DEFAULT_Y_PAD;
	int _iGridSize = DEFAULT_GRID_SIZE;

private:
	HPEN _hGridPen;
	HPEN _hPathPen;
	HFONT _hMenuFont;
	HFONT _hDataFont;
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
	Map* _pMap = nullptr;
	NodeMgr* _pNodeMgr = nullptr;
};

