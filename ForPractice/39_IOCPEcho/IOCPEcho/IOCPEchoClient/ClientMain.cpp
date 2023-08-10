#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_IP "127.0.0.1"
#define dfSERVER_PORT 6000
#define dfBUFSIZE 10

SOCKET g_Socket;
bool g_Shutdown;

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

    /*
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
    */

    PingPongTest();

    closesocket(g_Socket);
    WSACleanup();
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

unsigned int WINAPI SendThread(void* arg)
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

        //Sleep(1000);
    }

    return 0;
}

unsigned int WINAPI RecvThread(void* arg)
{
    while (!g_Shutdown)
    {
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

    return 0;
}

