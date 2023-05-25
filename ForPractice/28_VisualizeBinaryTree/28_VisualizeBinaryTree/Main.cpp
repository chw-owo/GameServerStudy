#include "framework.h"
#include "Main.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console" )

#define MAX_LOADSTRING 100
#define INPUT_LEN 8

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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY28VISUALIZEBINARYTREE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY28VISUALIZEBINARYTREE);
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

HPEN g_hPen;

HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

BasicBinaryTree<int> tree;

void DrawTree()
{
    HPEN hPen = (HPEN)SelectObject(g_hMemDC, g_hPen);
    tree.DrawAllNode(g_hMemDC);
    SelectObject(g_hMemDC, hPen);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:

        printf("1. Insert Node\n"
            "2. Insert Random Node\n"
            "3. Delete Node\n"
            "4. Clear Console\n\n"
            "Choose number\n");

        srand(time(NULL));
        g_hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

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
        case VK_NUMPAD1:
            {
                char nodeInput[INPUT_LEN] = { '\0', };
                printf("Enter Number to Insert\n");
                rewind(stdin);
                fgets(nodeInput, INPUT_LEN, stdin);
                int node = atoi(nodeInput);
                tree.InsertNode(node);
                InvalidateRect(hWnd, NULL, TRUE);

                printf("\n1. Insert Node\n"
                    "2. Insert Random Node\n"
                    "3. Delete Node\n"
                    "4. Clear Console\n\n"
                    "Choose number\n");
            }
            break;

        case VK_NUMPAD2:
            break;

        case VK_NUMPAD3:
            {
                char nodeInput[INPUT_LEN] = { '\0', };
                printf("Enter Number to Delete\n");
                rewind(stdin);
                fgets(nodeInput, INPUT_LEN, stdin);
                int node = atoi(nodeInput);
                tree.DeleteNode(node);
                InvalidateRect(hWnd, NULL, TRUE);

                printf("\n1. Insert Node\n"
                    "2. Insert Random Node\n"
                    "3. Delete Node\n"
                    "4. Clear Console\n\n"
                    "Choose number\n");
            }
            break;

        case VK_NUMPAD4:
            system("cls");
            break;

        default:
            break;
        }
    }
    break;

    case WM_PAINT:

        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        DrawTree();

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
