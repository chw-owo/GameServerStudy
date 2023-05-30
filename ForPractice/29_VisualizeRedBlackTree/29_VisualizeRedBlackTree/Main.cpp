#include "framework.h"
#include "Main.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console" )

#define MAX_LOADSTRING 100
#define INPUT_LEN 16

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

int g_iXPad = 0;
int g_iYPad = 0;
HPEN g_hBlackPen;
HPEN g_hRedPen;

HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;

RedBlackTree<int> tree;

void DrawTree()
{
    tree.DrawAllNode(g_hMemDC, g_hBlackPen, g_hRedPen, g_iXPad, g_iYPad);
}

void InsertRandomNode(int node)
{
    if (node <= RAND_MAX)
    {
        int* data = new int[node];

        for (int i = 0; i < node; i++)
        {
            data[i] = i;
        }
        for (int i = 0; i < node; i++)
        {
            int rand1 = (rand() % node);
            while (data[rand1] == INT_MAX)
            {
                rand1 = (rand() % node);
            }
            tree.InsertNode(data[rand1]);
            data[rand1] = INT_MAX;
        }

        delete[] data;
    }
    else
    {
        int* data = new int[node];

        for (int i = 0; i < node; i++)
        {
            data[i] = i;
        }
        for (int i = 0; i < node; i++)
        {
            int rand1 = rand();
            int rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= node;

            while (data[rand1] == INT_MAX)
            {
                rand1 = rand();
                rand2 = rand();
                rand1 = rand1 << 7;
                rand1 |= rand2;
                rand1 %= node;
            }

            tree.InsertNode(data[rand1]);
            data[rand1] = INT_MAX;
        }
        delete[] data;
    }
}

void DeleteRandomNode(int node)
{
    int max = tree.GetTreeSize();
    int* data = new int[max];
    tree.GetAllNode(data);

    if (node <= RAND_MAX)
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = (rand() % max);
            while (data[rand1] == INT_MAX)
            {
                rand1 = (rand() % max);
            }
            tree.DeleteNode(data[rand1]);
            data[rand1] = INT_MAX;
        }
    }
    else
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = rand();
            int rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= max;

            while (data[rand1] == INT_MAX)
            {
                rand1 = rand();
                rand2 = rand();
                rand1 = rand1 << 7;
                rand1 |= rand2;
                rand1 %= max;
            }

            tree.DeleteNode(data[rand1]);
            data[rand1] = INT_MAX;
        }
    }
    delete[] data;
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
            "4. Delete Random Node\n"
            "5. Clear Console\n\n"
            "Choose number\n");

        srand(500);
        g_hBlackPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        g_hRedPen = CreatePen(PS_SOLID, 1, RGB(200, 0, 0));

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
            g_iYPad += 3;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_DOWN:
            g_iYPad -= 3;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_LEFT:
            g_iXPad += 3;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_RIGHT:
            g_iXPad -= 3;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case VK_NUMPAD1:
        case 0x31:
        {
            char nodeInput[INPUT_LEN] = { '\0', };
            printf("Enter Number to Insert\n");
            rewind(stdin);
            fgets(nodeInput, INPUT_LEN, stdin);
            int node = atoi(nodeInput);
            tree.InsertNode(node);

            InvalidateRect(hWnd, NULL, TRUE);

            tree.PrintAllNodeData();
            int max = tree.GetTreeSize();
            int* data = new int[max];
            tree.GetAllNode(data);
            printf("\ntotal size: %d\n", max);
            for (int i = 0; i < max; i++)
                printf("%d ", data[i]);
            printf("\n");

            printf("\n1. Insert Node\n"
                "2. Insert Random Node\n"
                "3. Delete Node\n"
                "4. Delete Random Node\n"
                "5. Clear Console\n\n"
                "Choose number\n");
        }
        break;

        case VK_NUMPAD2:
        case 0x32:
        {
            char nodeInput[INPUT_LEN] = { '\0', };
            printf("Enter Node Count to Insert (MAX: %d)\n", INT_MAX / 2);
            rewind(stdin);
            fgets(nodeInput, INPUT_LEN, stdin);
            int node = atoi(nodeInput);

            if (node > INT_MAX / 2 || node < 0)
                node = INT_MAX / 2;

            printf("Requested Count: %d\n", node);

            InsertRandomNode(node);

            tree.PrintAllNodeData();
            int max = tree.GetTreeSize();
            int* data = new int[max];
            printf("\ntotal size: %d\n", max);
            tree.GetAllNode(data);
            printf("\n");
            for (int i = 0; i < max; i++)
                printf("%d ", data[i]);
            printf("\n");

            InvalidateRect(hWnd, NULL, TRUE);

            printf("\n1. Insert Node\n"
                "2. Insert Random Node\n"
                "3. Delete Node\n"
                "4. Delete Random Node\n"
                "5. Clear Console\n\n"
                "Choose number\n");
        }
        break;

        case VK_NUMPAD3:
        case 0x33:
        {
            char nodeInput[INPUT_LEN] = { '\0', };
            printf("Enter Number to Delete\n");
            rewind(stdin);
            fgets(nodeInput, INPUT_LEN, stdin);
            int node = atoi(nodeInput);
            tree.DeleteNode(node);

            tree.PrintAllNodeData();
            int max = tree.GetTreeSize();
            int* data = new int[max];
            printf("\ntotal size: %d\n", max);
            tree.GetAllNode(data);
            printf("\n");
            for (int i = 0; i < max; i++)
                printf("%d ", data[i]);
            printf("\n");

            InvalidateRect(hWnd, NULL, TRUE);

            printf("\n1. Insert Node\n"
                "2. Insert Random Node\n"
                "3. Delete Node\n"
                "4. Delete Random Node\n"
                "5. Clear Console\n\n"
                "Choose number\n");
        }
        break;

        case VK_NUMPAD4:
        case 0x34:
        {
            char nodeInput[INPUT_LEN] = { '\0', };
            printf("Enter Node Count to Delete (MAX: %d)\n", tree.GetTreeSize());
            rewind(stdin);
            fgets(nodeInput, INPUT_LEN, stdin);
            int node = atoi(nodeInput);

            if (node > tree.GetTreeSize() || node < 0)
                node = tree.GetTreeSize();

            printf("Requested Count: %d\n", node);

            DeleteRandomNode(node);

            tree.PrintAllNodeData();
            int max = tree.GetTreeSize();
            int* data = new int[max];
            printf("\ntotal size: %d\n", max);
            tree.GetAllNode(data);
            printf("\n");
            for (int i = 0; i < max; i++)
                printf("%d ", data[i]);
            printf("\n");

            InvalidateRect(hWnd, NULL, TRUE);

            printf("\n1. Insert Node\n"
                "2. Insert Random Node\n"
                "3. Delete Node\n"
                "4. Delete Random Node\n"
                "5. Clear Console\n\n"
                "Choose number\n");
        }
        break;

        case VK_NUMPAD5:
        case 0x35:
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

