#include "Map.h"
#include <iostream>


CMap::CMap()
{
    srand(unsigned short(200));

    _hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    _hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
    _hStartBrush = CreateSolidBrush(RGB(200, 0, 0));
    _hDestBrush = CreateSolidBrush(RGB(0, 200, 0));

    _pDestPos._X = GRID_WIDTH - 1;
    _pDestPos._Y = GRID_HEIGHT - 1;

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

CMap::~CMap()
{
}

void CMap::Draw(int xPos, int yPos)
{
    int iTileX = (xPos - _iXPad) / _iGridSize;
    int iTileY = (yPos - _iYPad) / _iGridSize;
    if (iTileX >= GRID_WIDTH)   iTileX = GRID_WIDTH - 1;
    if (iTileY >= GRID_HEIGHT)  iTileY = GRID_HEIGHT - 1;

    if (_bDraw && _bSelectStart)    
    {
        _pStartPos._X = iTileX;
        _pStartPos._Y = iTileY;
    }
    else if (_bDraw && _bSelectDest)
    {
        _pDestPos._X = iTileX;
        _pDestPos._Y = iTileY;
    }
    else if (_bDraw)                   
    {
        _chMap[iTileY][iTileX] = OBSTACLE;
    }
    else if (_bErase)                  
    {
        _chMap[iTileY][iTileX] = NONE;
    }
}

void CMap::DrawRandom()
{
    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            _chMap[i][j] = rand() % (OBSTACLE + 1);
        }
    }
}

void CMap::FindPath()
{
    _chMap[_pStartPos._X][_pStartPos._Y] = START;
    _chMap[_pDestPos._X][_pDestPos._Y] = DEST;

    // Find Path
}

void CMap::RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hGridPen);

    for (int i = 0; i <= GRID_WIDTH; i++)
    {
        MoveToEx(hdc, iX + _iXPad, _iYPad, NULL);
        LineTo(hdc, iX + _iXPad, GRID_HEIGHT * _iGridSize + _iYPad);
        iX += _iGridSize;
    }

    for (int i = 0; i <= GRID_HEIGHT; i++)
    {
        MoveToEx(hdc, _iXPad, iY + _iYPad, NULL);
        LineTo(hdc, GRID_WIDTH * _iGridSize + _iXPad, iY + _iYPad);
        iY += _iGridSize;
    }

    SelectObject(hdc, hOldPen);
}

void CMap::RenderColor(HDC hdc)
{
    int iX, iY;
    SelectObject(hdc, GetStockObject(NULL_PEN));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, _hTileBrush);

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            iX = j * _iGridSize;
            iY = i * _iGridSize;

            if (_chMap[i][j] == OBSTACLE)
            {
                Rectangle(hdc, iX + _iXPad, iY + _iYPad,
                    iX + _iGridSize + 2 + _iXPad,
                    iY + _iGridSize + 2 + _iYPad);
            }
        }
    }

    // Draw Start Point
    SelectObject(hdc, _hStartBrush);
    iX = _pStartPos._X * _iGridSize;
    iY = _pStartPos._Y * _iGridSize;
    Rectangle(hdc, iX + _iXPad, iY + _iYPad,
        iX + _iGridSize + 2 + _iXPad,
        iY + _iGridSize + 2 + _iYPad);

    // Draw Dest Point
    SelectObject(hdc, _hDestBrush);
    iX = _pDestPos._X * _iGridSize;
    iY = _pDestPos._Y * _iGridSize;
    Rectangle(hdc, iX + _iXPad, iY + _iYPad,
        iX + _iGridSize + 2 + _iXPad,
        iY + _iGridSize + 2 + _iYPad);

    SelectObject(hdc, hOldBrush);
}
