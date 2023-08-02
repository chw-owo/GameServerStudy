#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_IP "127.0.0.1"
#define dfSERVER_PORT 9000
#define dfBUFSIZE 128

int recvn(SOCKET sock, char* buf, int len, int flags)
{
    int recvd;
    char* ptr = buf;
    int left = len;

    while (left > 0)
    {
        recvd = recv(sock, ptr, left, flags);
        if (recvd == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        else if (recvd == 0)
        {
            break;
        }
        left -= recvd;
        ptr += recvd;
    }

    return (len - left);
}

int main()
{
    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // Create Socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
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
    int connectRet = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (connectRet == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
        return 0;
    }

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

        int sendRet = send(sock, buf, strlen(buf), 0);
        if (sendRet == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
            break;
        }
        printf("Success to Send %dbytes!\n", sendRet);

        int recvRet = recvn(sock, buf, sendRet, 0);
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

    closesocket(sock);
    WSACleanup();
    return 0;
}
