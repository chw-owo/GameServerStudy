#include "framework.h"
#include "Assignment20_CustomizeMap.h"
#define MAX_LOADSTRING 100
#define GRID_WIDTH 100
#define GRID_HEIGHT 50

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    MSG msg;
    HWND hWnd;
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASSIGNMENT20CUSTOMIZEMAP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ASSIGNMENT20CUSTOMIZEMAP);
    wcex.lpszClassName = L"ClassName";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassEx(&wcex);

    hWnd = CreateWindowW(L"ClassName",
        L"Title",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        nullptr, nullptr,
        hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

HPEN g_hGridPen;
HBRUSH g_hTileBrush;

int iGridSize = 16;
char g_chTile[GRID_HEIGHT][GRID_WIDTH] = { 0 };
bool g_bErase = false;
bool g_bDraw = false;

HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

void RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    for (int i = 0; i <= GRID_WIDTH; i++)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, GRID_HEIGHT * iGridSize);
        iX += iGridSize;
    }

    for (int i = 0; i <= GRID_HEIGHT; i++)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc, GRID_WIDTH * iGridSize, iY);
        iY += iGridSize;
    }
    SelectObject(hdc, hOldPen);
}

void RenderObstacle(HDC hdc)
{
    int iX, iY;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            iX = j * iGridSize;
            iY = i * iGridSize;

            if (g_chTile[i][j] == 1)         
                Rectangle(hdc, iX, iY, iX + iGridSize + 2, iY + iGridSize + 2);   
        }
    }
    SelectObject(hdc, hOldBrush);
}


void CreateRandomMap(HDC hdc)
{

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    int xPos, yPos;
    int iTileX, iTileY;

    switch (message)
    {
    case WM_CREATE:
        srand(time(NULL));
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);     
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        break;

    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            iGridSize++;
        else if(GET_WHEEL_DELTA_WPARAM(wParam) < 0)
            iGridSize--;
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_LBUTTONDOWN:
        g_bDraw = true;
        break;

    case WM_LBUTTONUP:
        g_bDraw = false;
        break;

    case WM_RBUTTONDOWN:
        g_bErase = true;
        break;

    case WM_RBUTTONUP:
        g_bErase = false;
        break;

    case WM_MOUSEMOVE:

        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);
        iTileX = xPos / iGridSize;
        iTileY = yPos / iGridSize;
        if (iTileX >= GRID_WIDTH)   iTileX = GRID_WIDTH - 1;
        if (iTileY >= GRID_HEIGHT)  iTileY = GRID_HEIGHT - 1;

        if (g_bDraw)    g_chTile[iTileY][iTileX] = 1;
        if (g_bErase)   g_chTile[iTileY][iTileX] = 0;   
        InvalidateRect(hWnd, NULL, false);
       
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            for (int i = 0; i < GRID_HEIGHT; i++)
            {
                for (int j = 0; j < GRID_WIDTH; j++)
                {
                    g_chTile[i][j] = rand() % 2;
                }
            }
            InvalidateRect(hWnd, NULL, false);
            break;

        default:
            break;
        }
        break;

    case WM_PAINT:
        
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        RenderObstacle(g_hMemDC);
        RenderGrid(g_hMemDC);

        hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right,
            g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);

        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
