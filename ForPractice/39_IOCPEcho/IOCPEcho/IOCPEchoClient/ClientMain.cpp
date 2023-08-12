#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
using namespace std;

#define dfSERVER_IP "127.0.0.1"
#define dfSERVER_PORT 6000
#define dfBUFSIZE 10

SOCKET g_Socket;
bool g_Shutdown;
int g_SendCnt;
int g_RecvCnt;

void PingPongTest();
unsigned int WINAPI SendThread(void* arg);
unsigned int WINAPI RecvThread(void* arg);

int main()
{
    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // Create Socket
    g_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_Socket == INVALID_SOCKET)
    {
        printf("Error! %s(%d)\n", __func__, __LINE__);
        return 0;
    }

    // Connect Socket to Server
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, dfSERVER_IP, &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(dfSERVER_PORT);

    int connectRet = connect(g_Socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (connectRet == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
        return 0;
    }

    HANDLE threads[2];

    threads[0] = (HANDLE)_beginthreadex(NULL, 0, SendThread, 0, 0, nullptr);
    threads[1] = (HANDLE)_beginthreadex(NULL, 0, RecvThread, 0, 0, nullptr);

    while(!g_Shutdown)
    {
        if (GetAsyncKeyState(VK_BACK))
            g_Shutdown = true;
    }

    WaitForMultipleObjects(2, threads, true, INFINITE);
    ::printf("\nAll Thread Terminate!\n");
    ::printf("RecvCnt: %d, SendCnt: %d\n", g_RecvCnt, g_SendCnt);


    //PingPongTest();

    closesocket(g_Socket);
    WSACleanup();
    Sleep(INFINITE);
    return 0;
}

#define dfPACKET_LEN 10
#define dfPAYLOAD_LEN 8

#pragma pack(1)
struct Packet
{
    short len;
    __int64 data;
};
#pragma pack(pop)

void PingPongTest()
{
    int data = 0;
    Packet sendPacket;
    sendPacket.len = dfPAYLOAD_LEN;

    while (!g_Shutdown)
    {
        sendPacket.data = data++;
        int sendRet = send(g_Socket, (char*)&sendPacket, dfPACKET_LEN, 0);
        if (sendRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        printf("Success to Send %dbytes!\n", sendRet);

        Packet recvPacket;
        int recvRet = recv(g_Socket, (char*)&recvPacket, dfPACKET_LEN, 0);
        if (recvRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        else if (recvRet == 0)
        {
            break;
        }

        printf("Success to Recv %d bytes!\n", recvRet);
        printf(" - %llu\n", recvPacket.data);
    }
}

queue<__int64> dataQ;
unsigned int WINAPI SendThread(void* arg)
{
    int data = 0;
    Packet sendPacket;
    sendPacket.len = dfPAYLOAD_LEN;

    while (!g_Shutdown)
    {
        g_SendCnt++;
        sendPacket.data = data++;
        int sendRet = send(g_Socket, (char*)&sendPacket, dfPACKET_LEN, 0);
        if (sendRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        //printf("Success to Send %llu\n", sendPacket.data);

        //dataQ.push(sendPacket.data);

        /*
        if (g_RecvCnt + 200 < g_SendCnt)
        {
            g_Shutdown = true;
            break;
        }
        */
    }

    printf("Send Thread Terminate\n");
    return 0;
}

unsigned int WINAPI RecvThread(void* arg)
{
    int data = 0;
    while (!g_Shutdown)
    {
        g_RecvCnt++;
        Packet recvPacket;
        int recvRet = recv(g_Socket, (char*)&recvPacket, dfPACKET_LEN, 0);
        if (recvRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        else if (recvRet == 0)
        {
            printf("Recv Return 0\n");
            break;
        }

        if (data == recvPacket.data)
        {
            data++;
            printf("Success to Recv %llu\n" , recvPacket.data);
        }
        else
        {
            printf("Lost Data! expected: %llu, recv: %llu\n", 
                data, recvPacket.data);
            g_Shutdown = true;
            break;
        }
    }

    printf("Recv Thread Terminate\n");
    return 0;
}

