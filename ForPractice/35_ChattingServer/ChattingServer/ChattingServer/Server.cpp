#include "Server.h"
#include <wchar.h>
#include "Main.h"
#include "Protocol.h"
#include "SerializePacket.h"
#include "CreateSCPacket.h"

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
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	_time.tv_sec = 0;
	_time.tv_usec = 0;

	printf("Network Setting Complete!\n");
}

Server::~Server()
{
	vector<User*>::iterator userIter = _userArray.begin();
	for (; userIter != _userArray.end(); userIter++)
	{
		User* pUser = *userIter;
		userIter = _userArray.erase(userIter);
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
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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
		printf("Error! Func %s Line %d: fail new\n", __func__, __LINE__);
		return;
	}

	pSession->_socket = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		delete pSession;
		return;
	}

	pSession->SetSessionAlive();
	pSession->_addr = clientaddr;
	_allSessions.push_back(pSession);
	CreateUser(pSession);
}

void Server::RecvProc(Session* pSession)
{
	unordered_map<Session*, User*>::iterator userIter = _SessionUserMap.find(pSession);
	User* user = userIter->second;

	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			user->SetUserDead();
			return;
		}
	}
	else if (recvRet == 0)
	{
		user->SetUserDead();
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
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
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			user->SetUserDead();
			return;
		}

		if ((char)header.byCode != (char)dfPACKET_CODE)
		{
			printf("Error! Wrong Header Code! %x - Func %s Line %d\n", header.byCode, __func__, __LINE__);
			user->SetUserDead();
			return;
		}

		if (useSize < dfHEADER_SIZE + header.wPayloadSize)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			user->SetUserDead();
			return;
		}

		Handle_RecvPacket(user, header.wMsgType);
		useSize = pSession->_recvBuf.GetUseSize();
	}
}

void Server::Handle_RecvPacket(User* user, WORD type)
{
	switch (type)
	{
	case df_REQ_LOGIN:
	{
		LOGIN_RESULT ret = Handle_REQ_LOGIN(user);
		SerializePacket buffer;
		int packetSize;

		switch (ret)
		{
		case LOGIN_RESULT::OK:
			packetSize = CreatePacket_RES_LOGIN(&buffer, df_RESULT_LOGIN_OK, user->_ID);
			break;
		case LOGIN_RESULT::DNICK:
			packetSize = CreatePacket_RES_LOGIN(&buffer, df_RESULT_LOGIN_DNICK, user->_ID);
			break;
		case LOGIN_RESULT::MAX:
			packetSize = CreatePacket_RES_LOGIN(&buffer, df_RESULT_LOGIN_MAX, user->_ID);
			break;
		case LOGIN_RESULT::ETC:
			packetSize = CreatePacket_RES_LOGIN(&buffer, df_RESULT_LOGIN_ETC, user->_ID);
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			break;
		}
		
		int enqueueRet = user->_pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
		if (enqueueRet != packetSize)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			user->SetUserDead();
			return;
		}
	}
	break;

	case df_REQ_ROOM_LIST:
	{
		RLIST_RESULT ret = Handle_REQ_ROOM_LIST(user);
		SerializePacket buffer;
		int packetSize;
		switch (ret)
		{
		case RLIST_RESULT::OK:
		{
			unordered_map<int, Room*>::iterator i = _roomMap.begin();
			for(;i!= _roomMap.end(); i++)
			{
				Room* room = i->second;
				packetSize = CreatePacket_RES_ROOM_LIST(&buffer, room->_ID,
					wcslen(room->_title), room->_title, room->_userArray.size(), user->_name);

				int enqueueRet = user->_pSession->_sendBuf.Enqueue(buffer.GetReadPtr(), packetSize);
				if (enqueueRet != packetSize)
				{
					printf("Error! Func %s Line %d\n", __func__, __LINE__);
					user->SetUserDead();
					return;
				}
			}
		}
			break;

		case RLIST_RESULT::ETC:
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			break;
		}
	}
	break;

	case df_REQ_ROOM_CREATE:
	{
		wchar_t* title;
		RCREATE_RESULT ret = Handle_REQ_ROOM_CREATE(user, title);
		CreateRoom(title); 
		// TO-DO: result를 얘에서 받아야겠구나
		// enum 만든 건 다 없애고 BYTE로 반환하도록 수정...

		switch (ret)
		{
		case RCREATE_RESULT::OK:
			//CreatePacket_RES_ROOM_CREATE(SerializePacket * buffer, 
			//			int roomID, short roomNameLen, wchar_t* roomName);
			break;
		case RCREATE_RESULT::DNICK:
			break;
		case RCREATE_RESULT::MAX:
			break;
		case RCREATE_RESULT::ETC:
			break;
		}
	}
	break;

	case df_REQ_ROOM_ENTER:
	{
		RENTER_RESULT ret = Handle_REQ_ROOM_ENTER(user);
		switch (ret)
		{
		case RENTER_RESULT::OK:
			break;
		case RENTER_RESULT::NOT:
			break;
		case RENTER_RESULT::MAX:
			break;
		case RENTER_RESULT::ETC:
			break;
		}
	}
	break;

	case df_REQ_CHAT:
	{
		CHAT_RESULT ret = Handle_REQ_CHAT(user);
		switch (ret)
		{
		case CHAT_RESULT::OK:
			break;
		case CHAT_RESULT::ETC:
			break;
		}
	}
	break;

	case df_REQ_ROOM_LEAVE:
	{
		RLEAVE_RESULT ret = Handle_REQ_ROOM_LEAVE(user);
		switch (ret)
		{
		case RLEAVE_RESULT::OK:
			LeaveRoom(user);
			break;
		case RLEAVE_RESULT::ETC:
			break;
		}
	}
	break;
	}
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
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			pSession->SetSessionDead();
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
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
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
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
					printf("Error! Func %s Line %d\n", __func__, __LINE__);
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
					printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}

void Server::CreateUser(Session* pSession)
{
	User* pUser = new User(_userID, pSession);
	_userArray.push_back(pUser);
	_SessionUserMap.insert(make_pair(pSession, pUser));
	_userID++;
}

void Server::DestroyDeadUser()
{
	vector<User*>::iterator i = _userArray.begin();
	for (; i != _userArray.end();)
	{
		if (!(*i)->GetUserAlive())
		{
			(*i)->_pSession->SetSessionDead();
			unordered_map<Session*, User*>::iterator iter = _SessionUserMap.find((*i)->_pSession);
			_SessionUserMap.erase(iter);
			User* pUser = *i;
			i = _userArray.erase(i);
			delete(pUser);
		}
		else
		{
			i++;
		}
	}
}

void Server::CreateRoom(wchar_t* title)
{
	Room* pRoom = new Room(_roomID, title);
	_roomMap.insert({ _roomID, pRoom });
	_roomID++;
	delete[] title;
}

void Server::EnterRoom(User* user, int roomID)
{
	unordered_map<int, Room*>::iterator iter = _roomMap.find(roomID);
	Room* pRoom = iter->second;
	pRoom->_userArray.push_back(user);
	user->_roomID = roomID;
}

void Server::LeaveRoom(User* user)
{
	unordered_map<int, Room*>::iterator roomIter = _roomMap.find(_roomID);
	Room* pRoom = roomIter->second;
	vector<User*>::iterator userIter = find(pRoom->_userArray.begin(), pRoom->_userArray.end(), user);

	if (userIter == pRoom->_userArray.end())
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	pRoom->_userArray.erase(userIter);
	user->_roomID = -1;

	if (pRoom->_userArray.empty())
		_roomMap.erase(roomIter);	
}

LOGIN_RESULT Server::Handle_REQ_LOGIN(User* user)
{
	// TO-DO: Check Invalid User
	wchar_t nullname[dfNICK_MAX_LEN] = { '/0', };
	if (wcscmp(user->_name, nullname) != 0)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	// Get Data from Packet
	SerializePacket buffer;
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN;
	int dequeueRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer.GetData((char*)user->_name, size);

	return LOGIN_RESULT::OK;
}

RLIST_RESULT Server::Handle_REQ_ROOM_LIST(User* user)
{
	// TO-DO: Check Invalid User
	return RLIST_RESULT::OK;
}

RCREATE_RESULT Server::Handle_REQ_ROOM_CREATE(User* user, wchar_t* title)
{
	// TO-DO: Check Invalid User

	// Get Data Size from Packet
	SerializePacket buffer;
	short titleLen = 0;

	int dequeueLenRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), sizeof(titleLen));
	if (dequeueLenRet != sizeof(titleLen))
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer.MoveWritePos(sizeof(titleLen));
	buffer >> titleLen;

	// Get Data from Packet
	title = new wchar_t[titleLen];
	int size = titleLen * sizeof(wchar_t);
	int dequeueRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer.GetData((char*)title, titleLen * sizeof(wchar_t));

	return RCREATE_RESULT::OK;
}

RENTER_RESULT Server::Handle_REQ_ROOM_ENTER(User* user)
{
	// TO-DO: Check Invalid User

	// Get Data from Packet
	SerializePacket buffer;
	int roomID;

	int size = sizeof(roomID);
	int dequeueRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer >> roomID;

	return RENTER_RESULT::OK;
}

CHAT_RESULT Server::Handle_REQ_CHAT(User* user)
{
	// TO-DO: Check Invalid User

	// Get Data Size from Packet
	SerializePacket buffer;
	short msgLen = 0;
	int dequeueLenRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), sizeof(msgLen));
	if (dequeueLenRet != sizeof(msgLen))
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer >> msgLen;
	buffer.MoveWritePos(sizeof(msgLen));

	// Get Data from Packet
	wchar_t* msg = new wchar_t[msgLen];
	int size = msgLen * sizeof(wchar_t);
	int dequeueRet = user->_pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		user->SetUserDead();
		return;
	}

	buffer.GetData((char*)msg, msgLen * sizeof(wchar_t));

	return CHAT_RESULT::OK;
}

RLEAVE_RESULT Server::Handle_REQ_ROOM_LEAVE(User* user)
{
	// TO-DO: Check Invalid User

	return RLEAVE_RESULT::OK;
}
