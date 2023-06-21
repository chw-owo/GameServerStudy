#include "framework.h"
#include "Main.h"
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY29VISUALIZEREDBLACKTREE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY29VISUALIZEREDBLACKTREE);
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


TreeTester* g_pTreeTester;

HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

bool g_bTree = true;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:

        g_pTreeTester = new TreeTester;

        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        break;

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_UP:
            g_pTreeTester->MoveUp();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_DOWN:
            g_pTreeTester->MoveDown();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_LEFT:
            g_pTreeTester->MoveLeft();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_RIGHT:
            g_pTreeTester->MoveRight();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD1:
        case 0x31:
            g_pTreeTester->SearchNode();
            break;

        case VK_NUMPAD2:
        case 0x32:
            g_pTreeTester->InsertNode();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD3:
        case 0x33:    
            g_pTreeTester->InsertRandomNodeUnder9999(); 
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD4:
        case 0x34:
            g_pTreeTester->InsertRandomNode(); 
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD5:
        case 0x35:
            g_pTreeTester->DeleteNode(); 
            InvalidateRect(hWnd, NULL, TRUE);
            break;         

        case VK_NUMPAD6:
        case 0x36:
            g_pTreeTester->DeleteRandomNode();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD7:
        case 0x37:
            g_pTreeTester->PrintNodeData();
            break;

        case VK_NUMPAD8:
        case 0x38:
            g_pTreeTester->PrintPathData();
            break;

        case VK_NUMPAD9:
        case 0x39:
            g_pTreeTester->TestTree();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD0:
        case 0x30:
            g_pTreeTester->SetCompareMode();
            break;

        case 0x51: // Q
            g_pTreeTester->PrintCompareResult();
            break;

        case 0x57: // W
            g_pTreeTester->ShiftTreeDraw();
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_ESCAPE:
            system("cls");
            g_pTreeTester->PrintMenu();
            break;

        default:
            break;
        }
    }
    break;

    case WM_PAINT:

        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        g_pTreeTester->DrawTree(g_hMemDC);
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

