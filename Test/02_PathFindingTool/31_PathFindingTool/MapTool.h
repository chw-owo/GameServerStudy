#pragma once
#include "NodeMgr.h"
#include <windows.h>


#define MOVESPEED 10
#define DEFAULT_X_PAD 40
#define DEFAULT_Y_PAD 70
#define DEFAULT_GRID_SIZE 10

class MapTool
{
public:
	MapTool();
	~MapTool();

public:
	void SetMap(int xPos, int yPos);
	void SetRandomObstacles();
	void ClearObstacles();
	void FillObstacles();
	
public:
	void Render(HDC hdc);

private:
	void RenderMenu(HDC hdc);
	void RenderGrid(HDC hdc);
	void RenderColor(HDC hdc);
	void RenderParent(HDC hdc);
	void RenderPath(HDC hdc);
	void RenderNodeInfo(HDC hdc);

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
	HFONT _hMenuFont;
	HFONT _hDataFont;

	HPEN _hGridPen;
	HPEN _hPathPen;
	HPEN _hCorrectedPathPen;
	HPEN _hParentPen;
	HPEN _hDiagCuzPen;

	HBRUSH _hObstacleBrush;
	HBRUSH _hOpenBrush;
	HBRUSH _hCloseBrush;
	HBRUSH _hStartBrush;
	HBRUSH _hDestBrush;
	HBRUSH _hDiagCuzBrush;
	HBRUSH _hCheckedBrush;
	HBRUSH _hCheckedDiagBrush;

private:
	bool _bDraw = false;
	bool _bErase = false;
	bool _bSelectStart = false;
	bool _bSelectDest = false;

private:
	NodeMgr* _pNodeMgr = nullptr;
};

