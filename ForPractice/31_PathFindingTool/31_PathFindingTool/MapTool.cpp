#include "MapTool.h"
#include <stdio.h>

MapTool::MapTool()
{
    srand(unsigned short(200));
    _pNodeMgr = NodeMgr::GetInstance();

    _hMenuFont = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        VARIABLE_PITCH | FF_ROMAN, TEXT("Arial"));
    _hDataFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        VARIABLE_PITCH | FF_ROMAN, TEXT("Arial"));

    _hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));     // Black
    _hPathPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));         // Red
    _hParentPen = CreatePen(PS_SOLID, 1, RGB(180, 0, 0));       // Dark Red

    _hObstacleBrush = CreateSolidBrush(RGB(100, 100, 100));     // Gray
    _hOpenBrush = CreateSolidBrush(RGB(0, 0, 255));             // Blue
    _hCloseBrush = CreateSolidBrush(RGB(255, 255, 0));          // Yellow
    _hStartBrush = CreateSolidBrush(RGB(255, 0, 0));            // Red
    _hDestBrush = CreateSolidBrush(RGB(0, 255, 0));             // Green
}

MapTool::~MapTool()
{

}

void MapTool::Draw(int xPos, int yPos)
{
    int iTileX = (xPos - _iXPad) / _iGridSize;
    int iTileY = (yPos - _iYPad) / _iGridSize;
    if (iTileX >= X_MAX)   iTileX = X_MAX - 1;
    if (iTileY >= Y_MAX)  iTileY = Y_MAX - 1;

    if (_bDraw && _bSelectStart)
    {
        _pNodeMgr->_pMap->SetMapState(iTileX, iTileY, Map::START);
    }
    else if (_bDraw && _bSelectDest)
    {
        _pNodeMgr->_pMap->SetMapState(iTileX, iTileY, Map::DEST);
    }
    else if (_bDraw)
    {
        _pNodeMgr->_pMap->SetMapState(iTileX, iTileY, Map::OBSTACLE);
    }
    else if (_bErase)
    {
        if(_pNodeMgr->_pMap->GetMapState(iTileX, iTileY) == Map::OBSTACLE)
            _pNodeMgr->_pMap->SetMapState(iTileX, iTileY, Map::NONE);
    }
}

void MapTool::DrawRandom()
{
    for (int i = 0; i < Y_MAX; i++)
    {
        for (int j = 0; j < X_MAX; j++)
        {
            _pNodeMgr->_pMap->SetMapState(j, i, (Map::STATE)(rand()%(Map::OBSTACLE+1)));
        }
    }
}


void MapTool::Render(HDC hdc)
{
    RenderMenu(hdc);
    RenderGrid(hdc);
    RenderColor(hdc);
   
    if (_iGridSize >= 32)
    {
        RenderNodeInfo(hdc);
    }

    if (_pNodeMgr->_pDest != nullptr)
    {
        RenderPath(hdc);
    }

    RenderParent(hdc);
}

#define RENDER_MENU_LEN 128
void MapTool::RenderMenu(HDC hdc)
{
    HFONT hOldFont = (HFONT)SelectObject(hdc, _hMenuFont);
    SetTextColor(hdc, RGB(200, 200, 200));
    WCHAR text[RENDER_MENU_LEN] = { '\0', };

    swprintf_s(text, RENDER_MENU_LEN,
        L"휠 : 맵 크기 조정 / 키보드 상하좌우: 맵 위치 조정 ");
    int iX = _iXPad;
    int iY = DEFAULT_Y_PAD * -1 + _iYPad;
    TextOutW(hdc, iX, iY, text, wcslen(text));

    wmemset(text, L'\0', RENDER_MENU_LEN);
    swprintf_s(text, RENDER_MENU_LEN,
        L"마우스 왼쪽 + 드래그: 맵 그리기 / 마우스 오른쪽 + 드래그 : 맵 지우기 / 스페이스 : 랜덤 맵 생성 / ");
    iY = DEFAULT_Y_PAD * ((float) -1 * 2 / 3) + _iYPad;
    TextOutW(hdc, iX, iY, text, wcslen(text));

    wmemset(text, L'\0', RENDER_MENU_LEN);
    swprintf_s(text, RENDER_MENU_LEN,
        L"A키 + 마우스 왼쪽 : 출발지 지정 / S키 + 마우스 왼쪽 클릭 : 목적지 지정 / 엔터: 길찾기 시작");
    iY = DEFAULT_Y_PAD * ((float) -1 / 3) + _iYPad;
    TextOutW(hdc, iX, iY, text, wcslen(text));

    SelectObject(hdc, hOldFont);
}

void MapTool::RenderGrid(HDC hdc)
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

void MapTool::RenderColor(HDC hdc)
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

            switch (_pNodeMgr->_pMap->GetMapState(j, i))
            {
            case Map::NONE:
                continue;

            case Map::OBSTACLE:
                SelectObject(hdc, _hObstacleBrush);
                break;

            case Map::OPEN:
                SelectObject(hdc, _hOpenBrush);
                break;

            case Map::CLOSE:
                SelectObject(hdc, _hCloseBrush);
                break;

            case Map::START:
                SelectObject(hdc, _hStartBrush);
                break;

            case Map::DEST:
                SelectObject(hdc, _hDestBrush);
                break;

            default:
                break;
            }

            Rectangle(hdc, iX + _iXPad, iY + _iYPad,
                iX + _iGridSize + 2 + _iXPad,
                iY + _iGridSize + 2 + _iYPad);
        }
    }

    SelectObject(hdc, hOldBrush);
}

void MapTool::RenderParent(HDC hdc)
{
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hParentPen);

    int iX = 0;
    int iY = 0;
    Node* pNode = nullptr;
    Node* pParent = nullptr;
    int nodeCnt = _pNodeMgr->_openList.size();

    for (int i = 0; i < nodeCnt; i++)
    {
        if (_pNodeMgr->_openList[i]->_pParent != nullptr)
        {
            pNode = _pNodeMgr->_openList[i];
            iX = (pNode->_pos._x + 0.5f) * _iGridSize + _iXPad;
            iY = (pNode->_pos._y + 0.5f) * _iGridSize + _iYPad;
            MoveToEx(hdc, iX, iY, NULL);

            pParent = pNode->_pParent;

            if(pNode->_pos._x - pParent->_pos._x > 0)
                iX = (pParent->_pos._x + 1) * _iGridSize + _iXPad;
            else if (pNode->_pos._x - pParent->_pos._x < 0)
                iX = pParent->_pos._x * _iGridSize + _iXPad;

            if (pNode->_pos._y - pParent->_pos._y > 0)
                iY = (pParent->_pos._y + 1) * _iGridSize + _iYPad;
            else if (pNode->_pos._y - pParent->_pos._y < 0)
                iY = pParent->_pos._y * _iGridSize + _iYPad;

            LineTo(hdc, iX, iY);
        }
    }

    nodeCnt = _pNodeMgr->_closeList.size();
    for (int i = 0; i < nodeCnt; i++)
    {
        if (_pNodeMgr->_closeList[i]->_pParent != nullptr)
        {
            pNode = _pNodeMgr->_closeList[i];
            iX = (pNode->_pos._x + 0.5f) * _iGridSize + _iXPad;
            iY = (pNode->_pos._y + 0.5f) * _iGridSize + _iYPad;
            MoveToEx(hdc, iX, iY, NULL);

            pParent = pNode->_pParent;

            if (pNode->_pos._x - pParent->_pos._x > 0)
                iX = (pParent->_pos._x + 1) * _iGridSize + _iXPad;
            else if (pNode->_pos._x - pParent->_pos._x < 0)
                iX = pParent->_pos._x * _iGridSize + _iXPad;

            if (pNode->_pos._y - pParent->_pos._y > 0)
                iY = (pParent->_pos._y + 1) * _iGridSize + _iYPad;
            else if (pNode->_pos._y - pParent->_pos._y < 0)
                iY = pParent->_pos._y * _iGridSize + _iYPad;

            LineTo(hdc, iX, iY);
        }
    }

    SelectObject(hdc, hOldPen);
}

void MapTool::RenderPath(HDC hdc)
{
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hPathPen);

    int x = 0;
    int y = 0;

    Node* pNode = _pNodeMgr->_pDest;
    while (pNode != _pNodeMgr->_pStart)
    {
        x = (pNode->_pos._x + 0.5f) * _iGridSize + _iXPad;
        y = (pNode->_pos._y + 0.5f) * _iGridSize + _iYPad;
        MoveToEx(hdc, x, y, NULL);

        pNode = pNode->_pParent;
        x = (pNode->_pos._x + 0.5f) * _iGridSize + _iXPad;
        y = (pNode->_pos._y + 0.5f) * _iGridSize + _iYPad;
        LineTo(hdc, x, y);
    }

    SelectObject(hdc, hOldPen);
}

#define RENDER_DATA_LEN 64
void MapTool::RenderNodeInfo(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    Node* pNode = nullptr;

    HFONT hOldFont = (HFONT)SelectObject(hdc, _hDataFont);
    SetTextColor(hdc, RGB(200, 200, 200));
    SetBkMode(hdc, TRANSPARENT);
    WCHAR text[RENDER_DATA_LEN];

    int nodeCnt = _pNodeMgr->_openList.size();
    for(int i = 0; i < nodeCnt; i++)
    {
        if (_pNodeMgr->_openList[i]->_pParent != nullptr)
        {
            pNode = _pNodeMgr->_openList[i];
            iX = pNode->_pos._x * _iGridSize + _iXPad;
            iY = pNode->_pos._y * _iGridSize + _iYPad;
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_g);
            TextOutW(hdc, iX, iY, text, wcslen(text));

            iY = pNode->_pos._y * _iGridSize + _iYPad + (_iGridSize * 1 / 3);
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_h);
            TextOutW(hdc, iX, iY, text, wcslen(text));

            iY = pNode->_pos._y * _iGridSize + _iYPad + (_iGridSize * 2 / 3);
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_f);
            TextOutW(hdc, iX, iY, text, wcslen(text));
        }
    }

    nodeCnt = _pNodeMgr->_closeList.size();
    for (int i = 0; i < nodeCnt; i++)
    {
        if (_pNodeMgr->_closeList[i]->_pParent != nullptr)
        {
            pNode = _pNodeMgr->_closeList[i];
            iX = pNode->_pos._x * _iGridSize + _iXPad;
            iY = pNode->_pos._y * _iGridSize + _iYPad;
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_g);
            TextOutW(hdc, iX, iY, text, wcslen(text));

            iY = pNode->_pos._y * _iGridSize + _iYPad + (_iGridSize * 1 / 3);
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_h);
            TextOutW(hdc, iX, iY, text, wcslen(text));

            iY = pNode->_pos._y * _iGridSize + _iYPad + (_iGridSize * 2 / 3);
            wmemset(text, L'\0', RENDER_DATA_LEN);
            swprintf_s(text, RENDER_DATA_LEN, L"%d", pNode->_f);
            TextOutW(hdc, iX, iY, text, wcslen(text));
        }
    }

    SelectObject(hdc, hOldFont);
}

void MapTool::SetScale(short wParam)
{
    if (wParam > 0)
        _iGridSize++;
    else if (wParam < 0)
        _iGridSize--;

    if (_iGridSize == 0) _iGridSize = 1;
}