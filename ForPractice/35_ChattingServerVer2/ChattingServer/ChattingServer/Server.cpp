#include "Server.h"
#include <wchar.h>
#include <stdio.h>
#include "Main.h"
#include "Protocol.h"
#include "SerializePacket.h"
#include "CreateSCPacket.h"

// TO-DO: new - delete 검토 필요
//		  이대로 돌리면 누수 왕창 나올 듯ㅎㅎ

// TO-DO: Leave, Delete 테스트 x
// TO-DO: Release 모드 테스트 x
// TO-DO: echo 코드 제작 x

Server::Server()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;
	_allSessions.reserve(dfDEFAULT_SESSIONS_MAX);

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		g_bShutdown = true;
		return;
	}

	// Set Listen Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Bind Socket
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);

	bindRet = bind(_listensock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	_time.tv_sec = 0;
	_time.tv_usec = 0;

	::printf("Network Setting Complete!\n");
}

Server::~Server()
{
	vector<User*>::iterator userIter = _allUsers.begin();
	for (; userIter != _allUsers.end(); userIter++)
	{
		User* pUser = *userIter;
		userIter = _allUsers.erase(userIter);
		delete(pUser);
	}

	vector<Session*>::iterator sessionIter = _allSessions.begin();
	for (; sessionIter != _allSessions.end(); sessionIter++)
	{
		Session* pSession = *sessionIter;
		sessionIter = _allSessions.erase(sessionIter);
		closesocket(pSession->_socket);
		delete(pSession);
	}

	closesocket(_listensock);
	WSACleanup();
}

Server* Server::GetInstance()
{
	static Server _networkManager;
	return &_networkManager;
}

void Server::Update()
{
	FD_SET rset;
	FD_SET wset;

	int idx = 1;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(_listensock, &rset);
	memset(_sessionArray, 0, sizeof(_sessionArray));

	vector<Session*>::iterator i = _allSessions.begin();
	if (i == _allSessions.end())
	{
		// Select Socket Set
		int selectRet = select(0, &rset, NULL, NULL, &_time);
		if (selectRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			g_bShutdown = true;
			return;
		}

		// Handle Selected Socket
		else if (selectRet > 0 && FD_ISSET(_listensock, &rset))
		{
			AcceptProc();
		}
	}
	else
	{
		for (; i != _allSessions.end(); i++)
		{
			FD_SET((*i)->_socket, &rset);
			if ((*i)->_sendBuf.GetUseSize() > 0)
				FD_SET((*i)->_socket, &wset);
			_sessionArray[idx] = (*i);
			idx++;

			if (idx == FD_SETSIZE)
			{
				SelectProc(rset, wset, FD_SETSIZE);

				idx = 1;
				FD_ZERO(&rset);
				FD_ZERO(&wset);
				FD_SET(_listensock, &rset);
				memset(_sessionArray, 0, sizeof(_sessionArray));
			}
		}

		if (idx > 1)
		{
			SelectProc(rset, wset, idx);
		}
	}
	DestroyDeadUser();
	DisconnectDeadSession();
}

void Server::SelectProc(FD_SET rset, FD_SET wset, int max)
{
	// Select Socket Set
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int selectRet = select(0, &rset, &wset, NULL, &_time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Handle Selected Socket
	else if (selectRet > 0)
	{
		if (FD_ISSET(_listensock, &rset))
			AcceptProc();

		for (int i = 1; i < max; i++)
		{
			if (FD_ISSET(_sessionArray[i]->_socket, &wset))
				SendProc(_sessionArray[i]);

			if (FD_ISSET(_sessionArray[i]->_socket, &rset))
				RecvProc(_sessionArray[i]);
		}
	}
}

void Server::AcceptProc()
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Session* pSession = new Session(_sessionID++);

	if (pSession == nullptr)
	{
		::printf("Error! Func %s Line %d: fail new\n", __func__, __LINE__);
		return;
	}

	pSession->_socket = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		delete pSession;
		return;
	}

	pSession->SetSessionAlive();
	pSession->_addr = clientaddr;
	_allSessions.push_back(pSession);
}

void Server::RecvProc(Session* pSession)
{
	User* pUser = nullptr;
	unordered_map<Session*, User*>::iterator userIter = _SessionUserMap.find(pSession);
	if (userIter != _SessionUserMap.end())
	{
		pUser = userIter->second;
	}

	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
		{  
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			if (pUser == nullptr)
				pSession->SetSessionDead();
			else
				pUser->SetUserDead();
			return;
		}
	}
	else if (recvRet == 0)
	{
		if (pUser == nullptr)
			pSession->SetSessionDead();
		else
			pUser->SetUserDead();
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		if (pUser == nullptr)
			pSession->SetSessionDead();
		else
			pUser->SetUserDead();
		return;
	}
	
	int useSize = pSession->_recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= dfHEADER_SIZE)
			break;

		st_PACKET_HEADER header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_SIZE);
		if (peekRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			if (pUser == nullptr)
				pSession->SetSessionDead();
			else
				pUser->SetUserDead();
			return;
		}

		if ((char)header.byCode != (char)dfPACKET_CODE)
		{
			::printf("Error! Wrong Header Code! %x - Func %s Line %d\n", header.byCode, __func__, __LINE__);
			if (pUser == nullptr)
				pSession->SetSessionDead();
			else
				pUser->SetUserDead();
			return;
		}

		if (useSize < dfHEADER_SIZE + header.wPayloadSize)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			if (pUser == nullptr)
				pSession->SetSessionDead();
			else
				pUser->SetUserDead();
			return;
		}

		if (!Handle_RecvPacket(pSession, pUser, header.wMsgType))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			if (pUser == nullptr)
				pSession->SetSessionDead();
			else
				pUser->SetUserDead();
			return;
		}

		useSize = pSession->_recvBuf.GetUseSize();
	}
}

bool Server::Handle_RecvPacket(Session* pSession, User* pUser, WORD type)
{
	switch (type)
	{
	case df_REQ_LOGIN:
	{
		::printf("===================================\n");
		::printf("<Login>\n");
		
		wchar_t* name = nullptr;
		if(!Handle_REQ_LOGIN(pSession, pUser, name))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_LOGIN\n");

		BYTE res = CreateUser(pSession, pUser, name);
		::printf("Complete CreateUser\n");

		SerializePacket buffer;
		int packetSize;
		if (res == df_RESULT_LOGIN_OK)
		{
			packetSize = CreatePacket_RES_LOGIN(&buffer, res, pUser->_ID);
		}
		else
		{
			packetSize = CreatePacket_RES_LOGIN(&buffer, res, 0);
		}

		::printf("Complete Packet RES_LOGIN\n");

		int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
		if (enqueueRet != packetSize)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Enqueue Packet to Send Buffer\n");

		delete[] name;
		::printf("===================================\n\n");
		return true;
	}
	break;

	case df_REQ_ROOM_LIST:
	{
		::printf("===================================\n");
		::printf("<Room List (User: %d)>\n", pUser->_ID);

		if(!Handle_REQ_ROOM_LIST(pUser))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_ROOM_LIST\n");

		int roomCnt = _roomIDMap.size();

		if (roomCnt < 0)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		else if (roomCnt == 0)
		{	
			SerializePacket buffer;
			int packetSize = CreatePacket_RES_ROOM_LIST(&buffer,
				roomCnt, nullptr, nullptr, nullptr, nullptr, nullptr);
			::printf("Complete CreatePacket RES_ROOM_LIST\n");

			int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
			if (enqueueRet != packetSize)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				return false;
			}
			::printf("Complete Enqueue Packet to Send Buffer\n");
			::printf("===================================\n\n");
		}
		else if (roomCnt > 0)
		{
			int* roomIDs = new int[roomCnt];
			short* roomNameLens = new short[roomCnt];
			BYTE* headcounts = new BYTE[roomCnt];

			int roomNamesTotalLen = 0;
			int userNamesTotalLen = 0;

			int idx = 0;
			Room* room;
			unordered_map<int, Room*>::iterator roomIter = _roomIDMap.begin();
			for (; roomIter != _roomIDMap.end(); roomIter++)
			{
				room = roomIter->second;
				memcpy(&roomIDs[idx], &(room->_ID), sizeof(int));
				short roomNameLen = wcslen(room->_title) * sizeof(wchar_t);
				memcpy(&roomNameLens[idx], &roomNameLen, sizeof(short));
				BYTE headcount = room->_userArray.size();
				memcpy(&headcounts[idx], &headcount, sizeof(BYTE));
				roomNamesTotalLen += roomNameLen;
				userNamesTotalLen += headcount;
				idx++;
			}

			if (userNamesTotalLen < 0)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				return false;
			}
			else if (userNamesTotalLen == 0)
			{
				wchar_t* roomNames = new wchar_t[roomNamesTotalLen + 1];
				wmemset(roomNames, L'\0', roomNamesTotalLen + 1);

				int i = 0;
				for (; roomIter != _roomIDMap.end(); roomIter++)
				{
					room = roomIter->second;
					wcscat_s(roomNames, roomNamesTotalLen, room->_title);
					::wprintf(L"(Debug) roomName[%d]: %s\n", i, room->_title);
					i++;
				}

				::wprintf(L"(Debug) roomNames: %s\n", roomNames);

				SerializePacket buffer;
				int packetSize = CreatePacket_RES_ROOM_LIST(&buffer,
					roomCnt, roomIDs, roomNameLens, roomNames, headcounts, nullptr);
				::printf("Complete CreatePacket RES_ROOM_LIST\n");

				int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
				if (enqueueRet != packetSize)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					return false;
				}
				::printf("Complete Enqueue Packet to Send Buffer\n");

				delete[] roomNames;

			}
			else if (userNamesTotalLen > 0)
			{
				wchar_t* roomNames = new wchar_t[roomNamesTotalLen + 1];
				wmemset(roomNames, L'\0', roomNamesTotalLen + 1);

				userNamesTotalLen *= dfNICK_MAX_LEN;
				wchar_t* userNames = new wchar_t[userNamesTotalLen + 1];
				wmemset(userNames, L'\0', userNamesTotalLen + 1);

				int i = 0;
				for (; roomIter != _roomIDMap.end(); roomIter++)
				{
					room = roomIter->second;
					wcscat_s(roomNames, roomNamesTotalLen, room->_title);
					::wprintf(L"(Debug) roomName[%d]: %s\n", i, room->_title);

					vector<User*>::iterator userIter = room->_userArray.begin();
					for (; userIter != room->_userArray.end(); userIter++)
					{
						wcscat_s(userNames, userNamesTotalLen, (*userIter)->_name);
						::wprintf(L"(Debug) userName[%d]: %s\n", i, (*userIter)->_name);
					}
					i++;
				}

				::wprintf(L"(Debug) roomNames: %s\n", roomNames);
				::wprintf(L"(Debug) userNames: %s\n", userNames);

				SerializePacket buffer;
				int packetSize = CreatePacket_RES_ROOM_LIST(&buffer,
					roomCnt, roomIDs, roomNameLens, roomNames, headcounts, userNames);
				::printf("Complete CreatePacket RES_ROOM_LIST\n");

				int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
				if (enqueueRet != packetSize)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					return false;
				}
				::printf("Complete Enqueue Packet to Send Buffer\n");

				delete[] userNames;
				delete[] roomNames;
			}

			delete[] roomIDs;
			delete[] roomNameLens;
			delete[] headcounts;

			::printf("===================================\n\n");
		}

		return true;
	}
	break;

	case df_REQ_ROOM_CREATE:
	{
		::printf("===================================\n");
		::printf("<Room Create (User: %d)>\n", pUser->_ID);
		short titleLen = 0;
		wchar_t* title = nullptr;
		if(!Handle_REQ_ROOM_CREATE(pUser, titleLen, title))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_ROOM_CREATE\n");

		int roomID = -1;
		BYTE res = CreateRoom(titleLen, title, roomID);
		::printf("Complete CreateRoom\n");

		int packetSize;
		SerializePacket buffer;
		if (res == df_RESULT_ROOM_CREATE_OK)
		{			
			packetSize = CreatePacket_RES_ROOM_CREATE(
				&buffer, res, roomID, titleLen * sizeof(wchar_t), title);

			::printf("Complete Packet RES_ROOM_CREATE\n");

			vector<User*>::iterator i = _allUsers.begin();
			for (; i != _allUsers.end(); i++)
			{
				int enqueueRet = (*i)->_pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
				if (enqueueRet != packetSize)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					return false;
				}
			}
			::printf("Complete Enqueue Packet to All Send Buffer\n");
		}
		else
		{
			packetSize = CreatePacket_RES_ROOM_CREATE(
				&buffer, res, 0, 0, nullptr);

			::printf("Complete Packet RES_ROOM_CREATE\n");

			int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
			if (enqueueRet != packetSize)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				return false;
			}
			::printf("Complete Enqueue Packet to Self Send Buffer\n");
		}
		
		delete[] title;

		::printf("===================================\n\n");
		return true;
	}
	break;

	case df_REQ_ROOM_ENTER:
	{
		::printf("===================================\n");
		::printf("<Room Enter (User: %d)>\n", pUser->_ID);

		int roomID;
		if (!Handle_REQ_ROOM_ENTER(pUser, roomID))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_ROOM_ENTER\n");

		Room* pRoom = nullptr;
		BYTE res = EnterRoom(pUser, roomID, pRoom);
		::printf("Complete EnterRoom\n");
		
		if(res != df_RESULT_ROOM_ENTER_OK)
		{
			SerializePacket buffer;
			int packetSize = CreatePacket_RES_ROOM_ENTER(
				&buffer, res, 0, 0, nullptr, 0, nullptr, nullptr);

			int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
			if (enqueueRet != packetSize)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				return false;
			}
			::printf("Complete Enqueue Packet to Send Buffer\n");
		}
		else
		{
			int headcount = pRoom->_userArray.size();
			wchar_t* userNames = new wchar_t[(headcount * dfNICK_MAX_LEN) + 1];
			wmemset(userNames, L'\0', headcount* dfNICK_MAX_LEN + 1);
			int* userNums = new int[headcount];
	
			int idx = 0;
			vector<User*>::iterator i = pRoom->_userArray.begin();
			for (; i != pRoom->_userArray.end(); i++)
			{
				wcscpy_s(&userNames[(idx * dfNICK_MAX_LEN)], 
					dfNICK_MAX_LEN, (*i)->_name);
				::wprintf(L"(Debug) userName: %s\n", (*i)->_name);
				memcpy(&userNums[idx], &((*i)->_ID), sizeof(int));
				idx++;
			}

			::wprintf(L"(Debug) userNames: %s\n", userNames);

			SerializePacket buffer1;
			int packetSize1 = CreatePacket_RES_ROOM_ENTER(&buffer1, res,
				roomID, wcslen(pRoom->_title) * sizeof(wchar_t), pRoom->_title,
				headcount, userNames, userNums);

			::printf("Complete Create Packet RES_ROOM_ENTER\n");

			int enqueueRet = pSession->_sendBuf.Enqueue(buffer1.GetReadPtr(), packetSize1);
			if (enqueueRet != packetSize1)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				return false;
			}
			::printf("Complete Enqueue Packet to Self Send Buffer\n");

			SerializePacket buffer2;
			
			int packetSize2 = CreatePacket_RES_USER_ENTER(&buffer2, pUser->_name, pUser->_ID);

			::printf("Complete Create Packet RES_USER_ENTER\n");

			i = pRoom->_userArray.begin();
			for (; i != pRoom->_userArray.end(); i++)
			{
				if (pUser->_ID == (*i)->_ID) continue;
				enqueueRet = (*i)->_pSession->_sendBuf.Enqueue(buffer2.GetReadPtr(), packetSize2);
				if (enqueueRet != packetSize2)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					return false;
				}
			}
			
			::printf("Complete Enqueue Packet to Others Send Buffer\n");

			::printf("===================================\n\n");
		}

		return true;
	}
	break;

	case df_REQ_CHAT:
	{
		::printf("===================================\n");
		::printf("<Chat (User: %d)>\n", pUser->_ID);

		short msgLen = 0;
		wchar_t* msg = nullptr;
		if(!Handle_REQ_CHAT(pUser, msgLen, msg))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_CHAT\n");
		
		int roomID = pUser->_roomID;
		unordered_map<int, Room*>::iterator roomIter = _roomIDMap.find(roomID);
		if (roomIter == _roomIDMap.end())
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		Room* pRoom = roomIter->second;
		vector<User*>::iterator userIter = pRoom->_userArray.begin();
		
		SerializePacket buffer;
		int packetSize = CreatePacket_RES_CHAT(
			&buffer, pUser->_ID, msgLen * sizeof(wchar_t), msg);
		::printf("Complete Packet RES_CHAT\n");

		for (; userIter != pRoom->_userArray.end(); userIter++)
		{
			if ((*userIter) == pUser) continue;
			int enqueueRet = (*userIter)->_pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
			if (enqueueRet != packetSize)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				(*userIter)->SetUserDead();
			}
		}
		::printf("Complete Enqueue Packet to Send Buffer\n");

		delete[] msg;
		::printf("===================================\n\n");

		return true;
	}
	break;

	case df_REQ_ROOM_LEAVE:
	{
		::printf("===================================\n");
		::printf("<Room Leave (User: %d)>\n", pUser->_ID);

		if(!Handle_REQ_ROOM_LEAVE(pUser))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Handle REQ_ROOM_LEAVE\n");

		int roomID = pUser->_roomID;
		if(!LeaveRoom(pUser))
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete LeaveRoom\n");

		SerializePacket buffer;
		int packetSize = CreatePacket_RES_ROOM_LEAVE(&buffer, pUser->_ID);
		::printf("Complete Packet RES_ROOM_LEAVE\n");

		int enqueueRet = pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
		if (enqueueRet != packetSize)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return false;
		}
		::printf("Complete Enqueue Packet to Self Send Buffer\n");
		
		// Send To Other Users
		unordered_map<int, Room*>::iterator roomIter = _roomIDMap.find(roomID);
		if (roomIter != _roomIDMap.end())
		{
			Room* pRoom = roomIter->second;
			vector<User*>::iterator userIter = pRoom->_userArray.begin();
			for (; userIter != pRoom->_userArray.end(); userIter++)
			{
				int enqueueRet = (*userIter)->_pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
				if (enqueueRet != packetSize)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*userIter)->SetUserDead();
				}
			}
		}
		::printf("Complete Enqueue Packet to Others Send Buffer\n");
		::printf("===================================\n\n");
		return true;
	}
	break;
	}

	::printf("Error! Func %s Line %d Case %d\n", __func__, __LINE__, type);
	return false;
}

void Server::SendProc(Session* pSession)
{
	if (pSession->_sendBuf.GetUseSize() <= 0)
		return;

	int sendRet = send(pSession->_socket,
		pSession->_sendBuf.GetReadBufferPtr(),
		pSession->_sendBuf.DirectDequeueSize(), 0);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			pSession->SetSessionDead();
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
		return;
	}
}


void Server::DisconnectDeadSession()
{
	vector<Session*>::iterator i = _allSessions.begin();
	for (; i != _allSessions.end();)
	{
		if (!(*i)->GetSessionAlive())
		{
			Session* pSession = *i;
			i = _allSessions.erase(i);
			closesocket(pSession->_socket);
			delete(pSession);
		}
		else
		{
			i++;
		}
	}
}


void Server::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
	}
}

void Server::EnqueueBroadcast(char* msg, int size, Session* pExpSession)
{
	// TO-DO: 몇천명 단위일 때 어떻게 다음 프레임으로 미룰 것인지

	int enqueueRet;

	if (pExpSession == nullptr)
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive())
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
	else
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive() && pExpSession->_socket != (*i)->_socket)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}

BYTE Server::CreateUser(Session* pSession, User*& pUser, wchar_t* name)
{
	if (_allUsers.size() >= dfUSER_MAX_CNT)
		return df_RESULT_LOGIN_MAX;

	vector<User*>::iterator i = _allUsers.begin();
	for (; i != _allUsers.end(); i++)
	{
		if (wcscmp((*i)->_name, name) == 0)
			return df_RESULT_LOGIN_DNICK;
	}

	pUser = new User(_userID, pSession, name);
	_allUsers.push_back(pUser);
	_SessionUserMap.insert(make_pair(pSession, pUser));
	_userID++;

	return df_RESULT_LOGIN_OK;
}

void Server::DestroyDeadUser()
{
	vector<User*>::iterator i = _allUsers.begin();
	for (; i != _allUsers.end();)
	{
		if (!(*i)->GetUserAlive())
		{
			if ((*i)->_roomID != -1)
			{
				// TO-DO: erase in room's user array
			}
			(*i)->_pSession->SetSessionDead();
			unordered_map<Session*, User*>::iterator iter = _SessionUserMap.find((*i)->_pSession);
			_SessionUserMap.erase(iter);
			User* pUser = *i;
			i = _allUsers.erase(i);
			delete(pUser);
		}
		else
		{
			i++;
		}
	}
}


BYTE Server::CreateRoom(short& titleLen, wchar_t* title, int& roomID)
{
	if(_roomIDMap.size() > dfROOM_MAX_CNT)
		return df_RESULT_ROOM_CREATE_MAX;

	unordered_map<int, Room*>::iterator i = _roomIDMap.begin();
	for (; i != _roomIDMap.end(); i++)
	{
		Room* pRoomTmp = i->second;
		if (wcscmp(title, pRoomTmp->_title) == 0)
		{
			return df_RESULT_ROOM_CREATE_DNICK;
		}
	}

	roomID = _roomID;
	Room* pRoom = new Room(roomID, titleLen + 1, title);
	_roomIDMap.insert({ roomID, pRoom });
	_roomID++;
	return df_RESULT_ROOM_CREATE_OK;
}

BYTE Server::EnterRoom(User* pUser, int roomID, Room*& pRoom)
{
	unordered_map<int, Room*>::iterator iter = _roomIDMap.find(roomID);
	if (iter == _roomIDMap.end())
		return df_RESULT_ROOM_ENTER_NOT;

	pRoom = iter->second;
	if (pRoom->_userArray.size() >= dfROOM_MAX_USER)
		return df_RESULT_ROOM_ENTER_MAX;

	pUser->_roomID = roomID;
	pRoom->_userArray.push_back(pUser);
	return df_RESULT_ROOM_ENTER_OK;
}

bool Server::LeaveRoom(User* pUser)
{
	int roomID = pUser->_roomID;
	unordered_map<int, Room*>::iterator roomIter = _roomIDMap.find(roomID);
	Room* pRoom = roomIter->second;
	vector<User*>::iterator userIter = find(pRoom->_userArray.begin(), pRoom->_userArray.end(), pUser);

	if (userIter == pRoom->_userArray.end())
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pUser->SetUserDead();
		return false;
	}

	pRoom->_userArray.erase(userIter);
	pUser->_roomID = -1;

	// Delete Room
	if (pRoom->_userArray.empty())
	{
		_roomIDMap.erase(roomIter);
		SerializePacket buffer;
		int packetSize = CreatePacket_RES_ROOM_DELETE(&buffer, roomID);
		EnqueueBroadcast(buffer.GetReadPtr(), packetSize);
	}

	return true;
}


bool Server::Handle_REQ_LOGIN(Session* pSession, User* pUser, wchar_t*& name)
{
	// 유효성 검사 1. 이미 로그인 한 유저
	if (pUser != nullptr)
	{
		::printf("이미 로그인 한 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	// Get Data from Packet
	SerializePacket buffer;
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN;
	int dequeueRet = pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);
	name = new wchar_t[dfNICK_MAX_LEN];
	buffer.GetData((char*)name, size);
	return true;
}

bool Server::Handle_REQ_ROOM_LIST(User* pUser)
{
	// 유효성 검사 1. 로그인 하지 않은 유저
	if (pUser == nullptr)
	{
		::printf("로그인 하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

bool Server::Handle_REQ_ROOM_CREATE(User* pUser, short& titleLen, wchar_t*& title)
{
	// 유효성 검사 1. 로그인 하지 않은 유저
	if (pUser == nullptr)
	{
		::printf("로그인 하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	// Get Data Size from Packet
	SerializePacket buffer;
	int dequeueLenRet = pUser->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), sizeof(titleLen));
	if (dequeueLenRet != sizeof(titleLen))
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueLenRet);
	buffer >> titleLen;

	// Get Data from Packet
	int size = titleLen;
	titleLen /= sizeof(wchar_t);
	title = new wchar_t[(titleLen + 1)];
	
	int dequeueRet = pUser->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer.GetData((char*)title, size);
	title[titleLen] = L'\0';
	
	return true;
}

bool Server::Handle_REQ_ROOM_ENTER(User* pUser, int& roomID)
{
	// 유효성 검사 1. 로그인 하지 않은 유저
	if (pUser == nullptr)
	{
		::printf("로그인 하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	// Get Data from Packet
	SerializePacket buffer;
	int size = sizeof(roomID);
	int dequeueRet = pUser->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);
	buffer >> roomID;

	// 유효성 검사 2. 이미 방에 속한 유저
	if (pUser->_roomID == roomID)
	{
		::printf("이미 방에 속한 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	return true;
}

bool Server::Handle_REQ_CHAT(User* pUser, short& msgLen, wchar_t*& msg)
{
	// 유효성 검사 1. 로그인 하지 않은 유저
	if (pUser == nullptr)
	{
		::printf("로그인 하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	// 유효성 검사 2. 방에 속하지 않은 유저
	if (pUser->_roomID == -1)
	{
		::printf("방에 속하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	// Get Data Size from Packet
	SerializePacket buffer;
	int dequeueLenRet = pUser->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), sizeof(msgLen));
	if (dequeueLenRet != sizeof(msgLen))
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueLenRet);
	buffer >> msgLen;
	
	// Get Data from Packet
	msgLen /= sizeof(wchar_t);
	msg = new wchar_t[msgLen];
	int size = msgLen * sizeof(wchar_t);
	int dequeueRet = pUser->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);
	buffer.GetData((char*)msg, size);
	return true;
}

bool Server::Handle_REQ_ROOM_LEAVE(User* pUser)
{
	// 유효성 검사 1. 로그인 하지 않은 유저
	if (pUser == nullptr)
	{
		::printf("로그인 하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}
	
	// 유효성 검사 2. 방에 속하지 않은 유저
	if (pUser->_roomID == -1)
	{
		::printf("방에 속하지 않은 유저: Func %s Line %d\n", __func__, __LINE__);
		return false;
	}

	return true;
}
