#include "PathFindingTool.h"
#include <iostream>
#include <list>
using namespace std;

CPathFindingTool::CPathFindingTool()
{
    _pathFinder = new Astar;
    memset(g_chMap, NONE, sizeof(g_chMap));
    srand(unsigned short(200));

    _hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

    _hObstacleBrush = CreateSolidBrush(RGB(100, 100, 100));
    _hOpenBrush = CreateSolidBrush(RGB(0, 0, 255)); // Blue
    _hCloseBrush = CreateSolidBrush(RGB(255, 255, 0)); // Yellow
    _hStartBrush = CreateSolidBrush(RGB(255, 0, 0)); // Red
    _hDestBrush = CreateSolidBrush(RGB(0, 255, 0)); // Green
    
    _pStartPos._x = 0;
    _pStartPos._y = 0;
    _pDestPos._x = X_MAX - 1;
    _pDestPos._y = Y_MAX - 1;

    g_chMap[_pStartPos._y][_pStartPos._x] = START;
    g_chMap[_pDestPos._y][_pDestPos._x] = DEST;

    printf(
        "<<메뉴얼>>\n\n"

        "마우스 휠 : 맵 크기 조정\n"
        "키보드 상하좌우: 맵 위치 조정\n"

        "스페이스: 랜덤 맵 생성\n\n"
        "마우스 왼쪽 클릭 + 드래그: 맵 그리기\n"
        "마우스 오른쪽 클릭 + 드래그: 맵 지우기\n"
        
        "A키 + 마우스 왼쪽 클릭 : 출발지 지정\n"
        "S키 + 마우스 왼쪽 클릭 : 목적지 지정\n"
        "엔터: 길찾기 시작\n\n"
    );
}

CPathFindingTool::~CPathFindingTool()
{
    delete _pathFinder;
}

void CPathFindingTool::Draw(int xPos, int yPos)
{
    int iTileX = (xPos - _iXPad) / _iGridSize;
    int iTileY = (yPos - _iYPad) / _iGridSize;
    if (iTileX >= X_MAX)   iTileX = X_MAX - 1;
    if (iTileY >= Y_MAX)  iTileY = Y_MAX - 1;

    if (_bDraw && _bSelectStart)    
    {
        g_chMap[_pStartPos._y][_pStartPos._x] = NONE;
        _pStartPos._x = iTileX;
        _pStartPos._y = iTileY;
        g_chMap[_pStartPos._y][_pStartPos._x] = START;
    }
    else if (_bDraw && _bSelectDest)
    {
        g_chMap[_pDestPos._y][_pDestPos._x] = NONE;
        _pDestPos._x = iTileX;
        _pDestPos._y = iTileY;
        g_chMap[_pDestPos._y][_pDestPos._x] = DEST;
    }
    else if (_bDraw)                   
    {
        g_chMap[iTileY][iTileX] = OBSTACLE;
    }
    else if (_bErase)                  
    {
        g_chMap[iTileY][iTileX] = NONE;
    }
}

void CPathFindingTool::DrawRandom()
{
    for (int i = 0; i < Y_MAX; i++)
    {
        for (int j = 0; j < X_MAX; j++)
        {
            g_chMap[i][j] = rand() % (OBSTACLE + 1);
        }
    }
}

void CPathFindingTool::StartFindPath()
{ 
    _bFindPath = true;
    _pathFinder->SetStartDestPoint(_pStartPos, _pDestPos);
}

void CPathFindingTool::FindPath()
{
    _pathFinder->FindPath();

    if (_pathFinder->_bComplete)
    {
        printf("Find Path Completely!\n");
        _bFindPath = false;
    }
}

void CPathFindingTool::Render(HDC hdc)
{
    RenderGrid(hdc);
    RenderColor(hdc);
    if ((_pathFinder->_resultPath).size() > 0)
        RenderPath(hdc);
}

void CPathFindingTool::RenderPath(HDC hdc)
{
    // _pathFinder->_resultPath;
    // pop_front ...
}

void CPathFindingTool::RenderPathFinderData(HDC hdc)
{
   
}

void CPathFindingTool::RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hGridPen);

    for (int i = 0; i <= X_MAX; i++)
    {
        MoveToEx(hdc, iX + _iXPad, _iYPad, NULL);
        LineTo(hdc, iX + _iXPad, Y_MAX * _iGridSize + _iYPad);
        iX += _iGridSize;
    }

    for (int i = 0; i <= Y_MAX; i++)
    {
        MoveToEx(hdc, _iXPad, iY + _iYPad, NULL);
        LineTo(hdc, X_MAX * _iGridSize + _iXPad, iY + _iYPad);
        iY += _iGridSize;
    }

    SelectObject(hdc, hOldPen);
}

void CPathFindingTool::RenderColor(HDC hdc)
{
    int iX, iY;
    SelectObject(hdc, GetStockObject(NULL_PEN));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, _hStartBrush);

    for (int i = 0; i < Y_MAX; i++)
    {
        for (int j = 0; j < X_MAX; j++)
        {
            iX = j * _iGridSize;
            iY = i * _iGridSize;

            switch(g_chMap[i][j])
            {
            case NONE:
                continue;

            case OBSTACLE:
                SelectObject(hdc, _hObstacleBrush);
                break;

            case OPEN:
                SelectObject(hdc, _hOpenBrush);
                break;

            case CLOSE:
                SelectObject(hdc, _hCloseBrush);
                break;

            case START:
                SelectObject(hdc, _hStartBrush);
                break;

            case DEST:
                SelectObject(hdc, _hDestBrush);
                break;
            }

            Rectangle(hdc, iX + _iXPad, iY + _iYPad,
                iX + _iGridSize + 2 + _iXPad,
                iY + _iGridSize + 2 + _iYPad);
        }
    }

    SelectObject(hdc, hOldBrush);
}

void CPathFindingTool::SetScale(short wParam)
{
    if (wParam > 0)
        _iGridSize++;
    else if (wParam < 0)
        _iGridSize--;

    if (_iGridSize == 0) _iGridSize = 1;
}