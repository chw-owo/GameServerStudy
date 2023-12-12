#include "CEchoGroup.h"
#include <tchar.h>

CEchoGroup::CEchoGroup(CServer* pNet)
{
	Setting((CNetServer*)pNet, 1);
	_userPool = new CTlsPool<CEchoUser>(dfUSER_MAX, true);
}

void CEchoGroup::Initialize()
{
}

void CEchoGroup::Update()
{
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
	// ::printf("%016llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	CEchoUser* user = _userPool->Alloc(sessionID);
	_usersMap.insert(make_pair(sessionID, user));
}

void CEchoGroup::OnLeaveGroup(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Echo::%s\n", sessionID, GetCurrentThreadId(), __func__);
	unordered_map<unsigned __int64, CEchoUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	_userPool->Free(mapIter->second);
	_usersMap.erase(mapIter);
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
	WORD msgType;
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
}

void CEchoGroup::OnSend(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Echo::%s (%d)\n", sessionID, GetCurrentThreadId(), __func__, sendSize);
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

void CEchoGroup::ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID)
{
	packet->SetGroup(this);
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

inline void CEchoGroup::HandlePacket_ECHO(CPacket* packet, CEchoUser* user)
{
	__int64 accountNo;
	LONGLONG sendTick;
	GetCSPacket_REQ_ECHO(packet, accountNo, sendTick);

	CPacket* sendPacket = CPacket::Alloc();
	SetSCPacket_RES_ECHO(sendPacket, accountNo, sendTick);
	ReqSendUnicast(sendPacket, user->_sessionID);
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
