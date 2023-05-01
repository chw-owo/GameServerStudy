#include "Network.h"
#include "Message.h"

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
		SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
	}

	newPlayer->state = BEFORE_SETTING;
	newPlayer->ID = gIDGenerator.AllocID();
	gPlayerList.push_back(newPlayer);
}


void RecvProc(Player* player)
{
	char recvBuf[MSGSIZE] = { 0 };
	if (player->state == ALIVE)
	{
		int recvRet = recv(player->sock, recvBuf, MSGSIZE, 0);
		if (recvRet == SOCKET_ERROR)
		{
			int err = ::WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				SetCursor();
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

		MSG_TYPE* pType = (MSG_TYPE*)recvBuf;

		switch (*pType)
		{
		case TYPE_MOVE:
		{
			MSG_MOVE* pMsg = (MSG_MOVE*)recvBuf;
			if (player->ID == pMsg->ID)
			{
				player->X = pMsg->X;
				player->Y = pMsg->Y;
				player->updated = true;
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
	if (player->state == ALIVE && player->updated == true)
	{
		MSG_MOVE MsgMove;
		MsgMove.ID = player->ID;
		MsgMove.X = player->X;
		MsgMove.Y = player->Y;
		SendBroadcast((char*)&MsgMove, player);
		player->updated = false;
	}
	else if (player->state == BEFORE_SETTING)
	{
		// Send <Allocate ID Message> to New Player
		MSG_ID MsgID;
		MsgID.ID = player->ID;
		SendUnicast((char*)&MsgID, player);

		// Send <Create New Player Message> to All Player
		MSG_CREATE MsgCreateNew;
		MsgCreateNew.ID = player->ID;
		MsgCreateNew.X = player->X;
		MsgCreateNew.Y = player->Y;
		SendBroadcast((char*)&MsgCreateNew);

		// Send <Create All Players Message> to New Player
		MSG_CREATE MsgCreateAll;
		for (CList<Player*>::iterator j = gPlayerList.begin(); j != gPlayerList.end(); j++)
		{
			MsgCreateAll.ID = (*j)->ID;
			MsgCreateAll.X = (*j)->X;
			MsgCreateAll.Y = (*j)->Y;
			SendUnicast((char*)&MsgCreateAll, player);
		}
		player->state = ALIVE;
	}
}

void SendUnicast(char* msg, Player* player)
{
	int sendRet = send(player->sock, msg, MSGSIZE, 0);
	if (sendRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			SetCursor();
			printf("Error! Line %d: %d\n", __LINE__, err);
			Disconnect(player);
		}
	}	
}

void SendBroadcast(char* msg, Player* expPlayer)
{
	int sendRet;
	if (expPlayer == nullptr)
	{
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if ((*i)->state == ALIVE)
			{
				sendRet = send((*i)->sock, msg, MSGSIZE, 0);
				if (sendRet == SOCKET_ERROR)
				{
					int err = ::WSAGetLastError();
					if (err != WSAEWOULDBLOCK)
					{
						SetCursor();
						printf("Error! Line %d: %d\n", __LINE__, err);
						Disconnect((*i));
					}
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
				sendRet = send((*i)->sock, msg, MSGSIZE, 0);
				if (sendRet == SOCKET_ERROR)
				{
					int err = ::WSAGetLastError();
					if (err != WSAEWOULDBLOCK)
					{
						SetCursor();
						printf("Error! Line %d: %d\n", __LINE__, err);
						Disconnect((*i));
					}
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
