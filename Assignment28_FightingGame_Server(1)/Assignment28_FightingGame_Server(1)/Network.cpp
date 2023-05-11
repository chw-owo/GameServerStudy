#include "Network.h"
#include "List.h"
#include "Player.h"
#include "PacketDefine.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdio.h>

FD_SET rset;
FD_SET wset;
SOCKET listen_sock;

#define SERVERIP L"0.0.0.0"
#define SERVERPORT 25000

bool NetworkInit()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	// Create Socket
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
		return false;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	bindRet = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
		return false;
	}

	// Listen
	listenRet = listen(listen_sock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
		return false;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(listen_sock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
		return false;
	}

	wprintf(L"Setting Complete!\n");
	return true;
}

bool NetworkProc()
{
	int err;
	int selectRet;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);
	for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
	{
		FD_SET((*i)->sock, &rset);
		if ((*i)->sendBuf.GetUseSize() > 0)
			FD_SET((*i)->sock, &wset);
	}

	// Select 
	selectRet = select(0, &rset, &wset, NULL, NULL);
	if (selectRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
		return false;
	}
	else if (selectRet > 0)
	{
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if (FD_ISSET((*i)->sock, &wset) && (*i)->alive)
				SendProc((*i));
		}

		if (FD_ISSET(listen_sock, &rset))
			AcceptProc();

		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if (FD_ISSET((*i)->sock, &rset) && (*i)->alive)
			{
				RecvProc((*i));
				SetBuffer((*i));
			}
		}

		if (deleted = true)
			DeleteDeadPlayers();
	}
}

void NetworkTerminate()
{
	//To-do: Delete All Player

	closesocket(listen_sock);
	WSACleanup();
}

void AcceptProc()
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Player* newPlayer = new Player;

	newPlayer->sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
	if (newPlayer->sock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		wprintf(L"Error! Line %d: %d\n", __LINE__, err);
	}

	newPlayer->alive = true;
	newPlayer->ID = gIDGenerator.AllocID();
	gPlayerList.push_back(newPlayer);

	// Send <Allocate ID Message> to New Player
	MSG_ID MsgID;
	MsgID.ID = newPlayer->ID;
	SendUnicast((char*)&MsgID, newPlayer);

	// Send <Create New Player Message> to All Player
	MSG_CREATE MsgCreateNew;
	MsgCreateNew.ID = newPlayer->ID;
	MsgCreateNew.X = newPlayer->X;
	MsgCreateNew.Y = newPlayer->Y;
	SendBroadcast((char*)&MsgCreateNew, newPlayer);

	// Send <Create All Players Message> to New Player
	MSG_CREATE MsgCreateAll;
	for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
	{
		MsgCreateAll.ID = (*i)->ID;
		MsgCreateAll.X = (*i)->X;
		MsgCreateAll.Y = (*i)->Y;
		SendUnicast((char*)&MsgCreateAll, newPlayer);
	}
}

void RecvProc(Player* player)
{
	char recvBuf[MAX_BUF_SIZE] = { '\0', };
	int recvRet = recv(player->sock, recvBuf, MAX_BUF_SIZE, 0);
	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			wprintf(L"Error! Line %d: %d\n", __LINE__, err);
			Disconnect(player);
			return;
		}
	}
	else if (recvRet == 0)
	{
		Disconnect(player);
		return;
	}

	int enqueueSize = player->recvBuf.Enqueue(recvBuf, recvRet);
	if (enqueueSize != recvRet)
	{
		wprintf(L"Error! Line %d: recv buffer enqueue\n", __LINE__);
		Disconnect(player);
		return;
	}

}

void SetBuffer(Player* player)
{
	while (player->recvBuf.GetUseSize() >= MSGSIZE)
	{
		char msgBuf[MSGSIZE];
		int dequeueRet = player->recvBuf.Dequeue(msgBuf, MSGSIZE);
		if (dequeueRet != MSGSIZE)
		{
			wprintf(L"Error! Line %d: recv buffer dequeue\n", __LINE__);
			Disconnect(player);
			return;
		}

		MSG_TYPE* pType = (MSG_TYPE*)msgBuf;

		switch (*pType)
		{
		case TYPE_MOVE:
		{
			MSG_MOVE* pMsg = (MSG_MOVE*)msgBuf;
			if (player->ID == pMsg->ID)
			{
				player->X = pMsg->X;
				player->Y = pMsg->Y;
				SendBroadcast((char*)pMsg, player);
			}
			break;
		}
		default:
			break;
		}
	}
}

void SendProc(Player* player)
{
	int msgBufSize = player->sendBuf.GetUseSize();
	if (msgBufSize <= 0)
	{
		return;
	}
	else if (msgBufSize % MSGSIZE != 0)
	{
		int remains = msgBufSize % MSGSIZE;
		msgBufSize -= remains;
	}

	char msgBuf[MAX_BUF_SIZE];
	int dequeueRet = player->sendBuf.Dequeue(msgBuf, msgBufSize);
	if (dequeueRet != msgBufSize)
	{
		wprintf(L"Error! Line %d: recv buffer dequeue\n", __LINE__);
		Disconnect(player);
		return;
	}

	int sendRet = send(player->sock, msgBuf, msgBufSize, 0);
	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			wprintf(L"Error! Line %d: %d\n", __LINE__, err);
			Disconnect(player);
			return;
		}
	}
}

void SendUnicast(char* msg, Player* player)
{
	int enqueueRet = player->sendBuf.Enqueue(msg, MSGSIZE);
	if (enqueueRet != MSGSIZE)
	{
		wprintf(L"Error! Line %d: send buffer enqueue error\n", __LINE__);
		Disconnect(player);
	}
}

void SendBroadcast(char* msg, Player* expPlayer)
{
	int enqueueRet;
	if (expPlayer == nullptr)
	{
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if ((*i)->alive)
			{
				enqueueRet = (*i)->sendBuf.Enqueue(msg, MSGSIZE);
				if (enqueueRet != MSGSIZE)
				{
					wprintf(L"Error! Line %d: send buffer enqueue error\n", __LINE__);
					Disconnect(*i);
				}
			}
		}
	}
	else
	{
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if (expPlayer->ID != (*i)->ID && (*i)->alive)
			{
				enqueueRet = (*i)->sendBuf.Enqueue(msg, MSGSIZE);
				if (enqueueRet != MSGSIZE)
				{
					wprintf(L"Error! Line %d: send buffer enqueue error\n", __LINE__);
					Disconnect(*i);
				}
			}
		}
	}

}

void Disconnect(Player* player)
{
	MSG_DELETE MsgDelete;
	MsgDelete.ID = player->ID;
	player->alive = false;
	SendBroadcast((char*)&MsgDelete);
	deleted = true;
}

void DeleteDeadPlayers()
{
	for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end();)
	{
		if (!(*i)->alive)
		{
			Player* player = *i;
			i = gPlayerList.erase(i);
			closesocket(player->sock);
			delete(player);
		}
		else
		{
			i++;
		}
	}
	deleted = false;
}
