#include "Network.h"
#include "Message.h"
#include "RingBuffer.h"

COORD stCoord;
HANDLE hConsole;
SOCKET listen_sock;

inline void SetCursor()
{
	stCoord.X = 0;
	stCoord.Y = YMAX;
	SetConsoleCursorPosition(hConsole, stCoord);
}

void AcceptProc()
{	
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Player* newPlayer = new Player;

	newPlayer->sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
	if (newPlayer->sock == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		//SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
	}

	newPlayer->state = BEFORE_SETTING;
	newPlayer->ID = gIDGenerator.AllocID();
	gPlayerList.push_back(newPlayer);
}


void RecvProc(Player* player)
{
	if (player->state == ALIVE)
	{
		char recvBuf[MAX_BUF_SIZE] = { '\0',};
		int recvRet = recv(player->sock, recvBuf, MAX_BUF_SIZE, 0);
		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				//SetCursor();
				printf("Error! Line %d: %d\n", __LINE__, err);
				Disconnect(player);
				return;
			}
		}
		else if (recvRet == 0)
		{
			Disconnect(player);
			return;
		}

		printf("%d: Recieve %d bytes\n", player->ID, recvRet);

		int enqueueSize = player->recvBuf.Enqueue(recvBuf, recvRet);
		if (enqueueSize != recvRet)
		{
			//SetCursor();
			printf("Error! Line %d: recv buffer enqueue\n", __LINE__);
			Disconnect(player);
			return;
		}

		while (player->recvBuf.GetUseSize() >= MSGSIZE)
		{
			char msgBuf[MSGSIZE];
			int dequeueRet = player->recvBuf.Dequeue(msgBuf, MSGSIZE);
			if(dequeueRet != MSGSIZE)
			{ 
				//SetCursor();
				printf("Error! Line %d: recv buffer dequeue\n", __LINE__);
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
					player->updated = true;
				}
				printf("%d: Get Move Message!\n", player->ID);
				break;
			}
			default:
				break;
			}
		}
	}
}

void SendProc(Player* player)
{
	if (player->state == ALIVE && player->updated == true)
	{
		MSG_MOVE MsgMove;
		MsgMove.ID = player->ID;
		MsgMove.X = player->X;
		MsgMove.Y = player->Y;
		SendBroadcast((char*)&MsgMove, player);
		player->updated = false;
		printf("%d: Set Move Message!\n", player->ID);
	}
	else if (player->state == BEFORE_SETTING)
	{
		// Send <Allocate ID Message> to New Player
		MSG_ID MsgID;
		MsgID.ID = player->ID;
		SendUnicast((char*)&MsgID, player);
		printf("%d: Set ID Message!\n", player->ID);

		// Send <Create New Player Message> to All Player
		MSG_CREATE MsgCreateNew;
		MsgCreateNew.ID = player->ID;
		MsgCreateNew.X = player->X;
		MsgCreateNew.Y = player->Y;
		SendBroadcast((char*)&MsgCreateNew);
		printf("%d: Set Create Message!\n", player->ID);

		// Send <Create All Players Message> to New Player
		MSG_CREATE MsgCreateAll;
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			MsgCreateAll.ID = (*i)->ID;
			MsgCreateAll.X = (*i)->X;
			MsgCreateAll.Y = (*i)->Y;
			SendUnicast((char*)&MsgCreateAll, player);
		}
		player->state = ALIVE;
		printf("%d: Set Create Message!\n", player->ID);
	}

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

	char* msgBuf = new char[msgBufSize]();
	int dequeueRet = player->sendBuf.Dequeue(msgBuf, msgBufSize);
	if (dequeueRet != msgBufSize)
	{
		//SetCursor();
		printf("Error! Line %d: recv buffer dequeue\n", __LINE__);
		Disconnect(player);
		delete[] msgBuf;
		return;
	}

	int sendRet = send(player->sock, msgBuf, msgBufSize, 0);
	delete[] msgBuf;
	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			//SetCursor();
			printf("Error! Line %d: %d\n", __LINE__, err);
			Disconnect(player);
			return;
		}
	}
	printf("%d: Send %d bytes\n", player->ID, sendRet);
}

void SendUnicast(char* msg, Player* player)
{
	int enqueueRet = player->sendBuf.Enqueue(msg, MSGSIZE);
	if (enqueueRet != MSGSIZE)
	{
		//SetCursor();
		printf("Error! Line %d: send buffer enqueue error\n", __LINE__);
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
			if ((*i)->state == ALIVE)
			{
				enqueueRet = (*i)->sendBuf.Enqueue(msg, MSGSIZE);
				if (enqueueRet != MSGSIZE)
				{
					//SetCursor();
					printf("Error! Line %d: send buffer enqueue error\n", __LINE__);
					Disconnect(*i);
				}
			}
		}
	}
	else
	{
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if (expPlayer->ID != (*i)->ID && (*i)->state == ALIVE)
			{
				enqueueRet = (*i)->sendBuf.Enqueue(msg, MSGSIZE);
				if (enqueueRet != MSGSIZE)
				{
					//SetCursor();
					printf("Error! Line %d: send buffer enqueue error\n", __LINE__);
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
	player->state = DEAD;
	SendBroadcast((char*)&MsgDelete);
	deleted = true;
}

void DeleteDeadPlayers()
{
	for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end();)
	{
		if ((*i)->state == DEAD)
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
