#include "framework.h"
#include "31_PathFindingTool.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console" )

#define MAX_LOADSTRING 100
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

MapTool* g_pMapTool = nullptr;
PathFindAlgorithm* g_pPathFindTool = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    MSG msg;
    HWND hWnd;
    WNDCLASSEXW wcex;

    g_pMapTool = new MapTool;

    //g_pPathFindTool = new AStar;
    //printf("Select AStar Algorithm\n");

    g_pPathFindTool = new JumpPointSearch;
    printf("Select JPS Algorithm\n");

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY31PATHFINDINGTOOL));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY31PATHFINDINGTOOL);
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
        if (g_pPathFindTool->GetFindPathOn())
        {
            g_pPathFindTool->FindPath();
            InvalidateRect(hWnd, NULL, false);
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

HBITMAP g_hMemDCBitMap;
HBITMAP g_hMemDCBitMap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitMap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitMap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitMap);
        break;

    case WM_MOUSEWHEEL:
        g_pMapTool->SetScale(GET_WHEEL_DELTA_WPARAM(wParam));
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_LBUTTONDOWN:
        g_pMapTool->SetDraw(true);
        g_pMapTool->SetMap(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONUP:
        g_pMapTool->SetDraw(false);
        break;

    case WM_RBUTTONDOWN:
        g_pMapTool->SetErase(true);
        g_pMapTool->SetMap(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_RBUTTONUP:
        g_pMapTool->SetErase(false);
        break;

    case WM_MOUSEMOVE:
        g_pMapTool->SetMap(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        // About Algorithm ===================================
        case 0x31: // Key Num 1
            printf("Select AStar Algorithm\n");
            delete g_pPathFindTool;
            g_pPathFindTool = new AStar;
            break;

        case 0x32: // Key Num 2
            printf("Select JPS Algorithm\n");
            delete g_pPathFindTool;
            g_pPathFindTool = new JumpPointSearch;
            break;

        // About Start/Dest ==================================

        case 0x33: // Key Num 3
            g_pMapTool->SelectStart(true);
            break;

        case 0x34: // Key Num 4
            g_pMapTool->SelectDest(true);
            break;

        // About Obstacle ====================================

        case 0x35: // Key Num 5
            g_pMapTool->SetRandomObstacles();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x36: // Key Num 6
            g_pMapTool->ClearObstacles();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x37: // Key Num 7
            g_pMapTool->FillObstacles();
            InvalidateRect(hWnd, NULL, false);
            break;

        // About Console ======================================

        case 0x51: // Key Q
            g_pPathFindTool->SetDebugCreateNode(true);
            break;
    
        case 0x57: // Key W
            g_pPathFindTool->SetDebugCreateNode(false);
            break;

        case 0x41: // Key A
            g_pPathFindTool->SetDebugFindCorner(true);
            break;

        case 0x53: // Key S
            g_pPathFindTool->SetDebugFindCorner(false);
            break;

        case 0x5A: // Key Z
            g_pPathFindTool->SetDebugOpenList(true);
            break;

        case 0x58: // Key X
            g_pPathFindTool->SetDebugOpenList(false);
            break;

        case 0x43: // Key C
            g_pPathFindTool->SetCorrectPath(true);
            break;

        case 0x56: // Key V
            g_pPathFindTool->SetCorrectPath(false);
            break;

        case VK_BACK:
            system("cls");
            break;

        // About Window =====================================

        case VK_UP:
            g_pMapTool->MoveUp();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_DOWN:
            g_pMapTool->MoveDown();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_LEFT:
            g_pMapTool->MoveLeft();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_RIGHT:
            g_pMapTool->MoveRight();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        // About Find Path =================================
        
        case VK_SPACE:
            g_pPathFindTool->FindPathStepInto();
            InvalidateRect(hWnd, NULL, false);
            break;

        case VK_RETURN:
            g_pPathFindTool->StartFindPath();
            InvalidateRect(hWnd, NULL, false);
            break;

        default:
            break;
        }
        break;

    case WM_KEYUP:
        switch (wParam)
        {
        // About Start/Dest ==================================

        case 0x33: // Key Num3
            g_pMapTool->SelectStart(false);
            break;

        case 0x34: // Key Num 4
            g_pMapTool->SelectDest(false);
            break;
        }
        break;

    case WM_PAINT:
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        g_pMapTool->Render(g_hMemDC);

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
