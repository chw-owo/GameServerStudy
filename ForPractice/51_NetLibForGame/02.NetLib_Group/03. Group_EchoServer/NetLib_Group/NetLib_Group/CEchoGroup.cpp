#include "CEchoGroup.h"
#include <tchar.h>

CEchoGroup::CEchoGroup(CServer* pNet)
{
	Setting((CNetServer*)pNet, 1);
}

void CEchoGroup::Initialize()
{
}

void CEchoGroup::Update()
{
	/*
	::printf(
		"Echo SendTPS: %d\n"
		"Echo RecvTPS: %d\n"
		"Echo EnterTPS: %d\n"
		"Echo LeaveTPS: %d\n"
		"Echo ReleaseTPS: %d\n\n",
		_sendTPS,
		_recvTPS,
		_enterTPS,
		_leaveTPS,
		_releaseTPS
	);
	*/
}

void CEchoGroup::Terminate()
{
}

void CEchoGroup::OnUpdate()
{
}

void CEchoGroup::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Echo Group Initialize\n");
	::wprintf(L"Echo Group Initialize\n");
}

void CEchoGroup::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Echo Group Terminate\n");
	::wprintf(L"Echo Group Terminate\n");
}

void CEchoGroup::OnEnterGroup(unsigned __int64 sessionID)
{
	::printf("%llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	_usersMap.insert(make_pair(sessionID, new CEchoUser(sessionID)));
	_enterTPS++;
}

void CEchoGroup::OnLeaveGroup(unsigned __int64 sessionID)
{
	::printf("%llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	unordered_map<unsigned __int64, CEchoUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	delete mapIter->second;
	_usersMap.erase(mapIter);
	_leaveTPS++;
}

void CEchoGroup::OnReleaseClient(unsigned __int64 sessionID)
{
	::printf("%llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	unordered_map<unsigned __int64, CEchoUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	delete mapIter->second;
	_usersMap.erase(mapIter);
	_releaseTPS++;
}

void CEchoGroup::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
	unordered_map<unsigned __int64, CEchoUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		CPacket::Free(packet);
		return;
	}

	CEchoUser* user = mapIter->second;
	::printf("%llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	short msgType = -1;
	*packet >> msgType;

	try
	{
		switch (msgType)
		{
		case en_PACKET_CS_GAME_REQ_ECHO:
			HandlePacket_ECHO(packet, user);
			break;

		default:
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
		{
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
		}
	}

	_recvTPS++;
}

void CEchoGroup::OnSend(unsigned __int64 sessionID, int sendSize)
{
	::printf("%llx (%d): Echo::%s (%d)\n", sessionID, GetCurrentThreadId(), __func__, sendSize);
	_sendTPS++;
}

void CEchoGroup::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

void CEchoGroup::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

inline void CEchoGroup::HandlePacket_ECHO(CPacket* packet, CEchoUser* user)
{
	__int64 accountNo;
	LONGLONG sendTick;
	GetCSPacket_REQ_ECHO(packet, accountNo, sendTick);

	CPacket* sendPacket = CPacket::Alloc();
	SetSCPacket_RES_ECHO(sendPacket, accountNo, sendTick);

	if (!SendPacket(user->_sessionID, sendPacket))
		CPacket::Free(sendPacket);
}

inline void CEchoGroup::GetCSPacket_REQ_ECHO(CPacket* packet, __int64& accountNo, LONGLONG& sendTick)
{
	*packet >> accountNo;
	*packet >> sendTick;
}

inline void CEchoGroup::SetSCPacket_RES_ECHO(CPacket* packet, __int64 accountNo, LONGLONG sendTick)
{
	*packet << (WORD)en_PACKET_CS_GAME_RES_ECHO;
	*packet << accountNo;
	*packet << sendTick;
}
