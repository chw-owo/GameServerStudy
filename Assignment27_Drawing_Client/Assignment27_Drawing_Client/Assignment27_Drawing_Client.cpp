#include "Assignment27_Drawing_Client.h"
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console" )

#define MAX_LOADSTRING 100
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HWND hWnd;
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASSIGNMENT27DRAWINGCLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ASSIGNMENT27DRAWINGCLIENT);
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

int iOldX = 0;
int iOldY = 0; 
int iCurX = 0; 
int iCurY = 0;

void Render(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
}

void ProcessMouseMove(WPARAM wParam, LPARAM lParam)
{
    // 아 오키... 마우스 뗐을 때는 그리지 않게끔 지정하면 됨 캬학 얼추 끝났다!!!
    if (wParam & MK_LBUTTON)
    {
        iCurX = GET_X_LPARAM(lParam);
        iCurY = GET_Y_LPARAM(lParam);

        DRAW_PACKET drawPacket;
        drawPacket.iStartX = iOldX;
        drawPacket.iStartY = iOldY;
        drawPacket.iEndX = iCurX;
        drawPacket.iEndY = iCurY;

        iOldX = iCurX;
        iOldY = iCurY;

        EnqueueSendData((char*)&drawPacket, (int)sizeof(DRAW_PACKET));
        SendUnicast();
    }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        if (!InitialSocket(hWnd))
            PostQuitMessage(0);
        break;

    case UM_SOCKET:
        ProcessSocketMessage(hWnd, message, wParam, lParam);
        break;

    case WM_MOUSEMOVE:
        ProcessMouseMove(wParam, lParam);
        break;

    case WM_PAINT:
        Render(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

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
