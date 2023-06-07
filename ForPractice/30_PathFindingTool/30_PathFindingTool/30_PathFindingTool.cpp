#include "framework.h"
#include "30_PathFindingTool.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console" )

#define MAX_LOADSTRING 100

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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY30PATHFINDINGTOOL));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY30PATHFINDINGTOOL);
    wcex.lpszClassName  = L"ClassName";
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
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

CPathFindingTool* g_pPathFindingTool = nullptr;
HBITMAP g_hMemDCBitMap;
HBITMAP g_hMemDCBitMap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    if (g_pPathFindingTool != nullptr &&
        g_pPathFindingTool->_bFindPath)
    {
        g_pPathFindingTool->FindPath();
    }

    switch (message)
    {
    case WM_CREATE:

        g_pPathFindingTool = new CPathFindingTool;

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitMap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitMap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitMap);

        break;

    case WM_MOUSEWHEEL:
        g_pPathFindingTool->SetScale(GET_WHEEL_DELTA_WPARAM(wParam));
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_LBUTTONDOWN:
        g_pPathFindingTool->SetDraw(true);
        g_pPathFindingTool->Draw(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONUP:
        g_pPathFindingTool->SetDraw(false);
        break;

    case WM_RBUTTONDOWN:
        g_pPathFindingTool->SetErase(true);
        g_pPathFindingTool->Draw(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_RBUTTONUP:
        g_pPathFindingTool->SetErase(false);
        break;

    case WM_MOUSEMOVE:
        g_pPathFindingTool->Draw(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case 0x41: // Key A
            g_pPathFindingTool->SelectStart(true);
            break;

        case 0x53: // Key S
            g_pPathFindingTool->SelectDest(true);
            break;

        case VK_UP:
            g_pPathFindingTool->MoveUp();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_DOWN:
            g_pPathFindingTool->MoveDown();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_LEFT:
            g_pPathFindingTool->MoveLeft();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_RIGHT:
            g_pPathFindingTool->MoveRight();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_SPACE:
            g_pPathFindingTool->DrawRandom();
            InvalidateRect(hWnd, NULL, false);
            break;

        case VK_RETURN:
            
            if (g_pPathFindingTool != nullptr && 
                g_pPathFindingTool->_bFindPath)
            {
                printf("It's already finding path\n");
                break;
            }
            else if (g_pPathFindingTool != nullptr &&
                !g_pPathFindingTool->_bFindPath)
            {
                g_pPathFindingTool->StartFindPath();
            }

            InvalidateRect(hWnd, NULL, false);

            break;

        default:
            break;
        }
        break;

    case WM_KEYUP:
        switch (wParam)
        {
        case 0x41: // Key A
            g_pPathFindingTool->SelectStart(false);
            break;

        case 0x53: // Key S
            g_pPathFindingTool->SelectDest(false);
            break;
        }
        break;

    case WM_PAINT:
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        g_pPathFindingTool->Render(g_hMemDC);

        hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right,
            g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitMap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitMap);
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        SelectObject(g_hMemDC, g_hMemDCBitMap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitMap);

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitMap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        g_hMemDCBitMap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitMap);
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
