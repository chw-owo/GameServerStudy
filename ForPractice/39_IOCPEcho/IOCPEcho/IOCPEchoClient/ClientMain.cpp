#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_IP "127.0.0.1"
#define dfSERVER_PORT 6000
#define dfBUFSIZE 10

SOCKET g_Socket;
void AutoTest();
void ManualTest();

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

    AutoTest();
    //ManualTest();

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

void AutoTest()
{
    int data = 0;
    Packet sendPacket;
    sendPacket.len = dfPAYLOAD_LEN;
    
    while (1)
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

void ManualTest()
{
    int len;
    char buf[dfBUFSIZE + 1];

    while (1)
    {
        // Setting Data to Send
        printf("\n[보낼 데이터] ");
        if (fgets(buf, dfBUFSIZE + 1, stdin) == NULL)
            break;

        len = strlen(buf);
        if (buf[len - 1] == '\n') buf[len - 1] = '\0';
        if (strlen(buf) == 0) break;

        int sendRet = send(g_Socket, buf, strlen(buf), 0);
        if (sendRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        printf("Success to Send %dbytes!\n", sendRet);

        int recvRet = recv(g_Socket, buf, sendRet, 0);
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

        buf[recvRet] = '\0';
        printf("Success to Recv %dbytes!\n", recvRet);
        printf(" - %s\n", buf);
    }
}
