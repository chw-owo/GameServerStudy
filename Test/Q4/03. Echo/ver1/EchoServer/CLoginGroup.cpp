#include "CLoginGroup.h"
#include "CServer.h"
#include <tchar.h>

CLoginGroup::CLoginGroup(CServer* pNet)
{
	_pEchoGroup = pNet->_pEchoGroup;
	Setting((CNetServer*)pNet, 0);
	_userPool = new CTlsPool<CLoginUser>(dfUSER_MAX, true);
}

void CLoginGroup::Initialize()
{
}

void CLoginGroup::Update()
{
}

void CLoginGroup::Terminate()
{
}

void CLoginGroup::OnUpdate()
{
}

void CLoginGroup::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Login Group Initialize\n");
	::wprintf(L"Login Group Initialize\n");
}

void CLoginGroup::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Login Group Terminate\n");
	::wprintf(L"Login Group Terminate\n");
}

void CLoginGroup::OnEnterGroup(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Login::%s\n", sessionID, GetCurrentThreadId(), __func__);
	CLoginUser* user = _userPool->Alloc(sessionID);
	_usersMap.insert(make_pair(sessionID, user));
}

void CLoginGroup::OnLeaveGroup(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Login::%s\n", sessionID, GetCurrentThreadId(), __func__);
	unordered_map<unsigned __int64, CLoginUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	_userPool->Free(mapIter->second);
	_usersMap.erase(mapIter);
}

void CLoginGroup::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
	unordered_map<unsigned __int64, CLoginUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %llx\n", _T(__FUNCTION__), __LINE__, sessionID);
		CPacket::Free(packet);
		return;
	}

	CLoginUser* user = mapIter->second;
	WORD msgType;
	*packet >> msgType;

	// ::printf("%016llx: Login::OnRecv %d\n", sessionID, msgType);

	try
	{
		switch (msgType)
		{
		case en_PACKET_CS_GAME_REQ_LOGIN:
			HandlePacket_LOGIN(packet, user);
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

void CLoginGroup::OnSend(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Login::%s\n", sessionID, GetCurrentThreadId(), __func__);
}

void CLoginGroup::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

void CLoginGroup::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

void CLoginGroup::ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID)
{
	packet->SetGroup(this);
	packet->AddUsageCount(1); 
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

inline void CLoginGroup::HandlePacket_LOGIN(CPacket* packet, CLoginUser* user)
{
	__int64 accountNo;
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	int version;
	GetCSPacket_REQ_LOGIN(packet, accountNo, sessionKey, version);
	delete[] sessionKey;

	BYTE status = LoginCheck(user);

	CPacket* sendPacket = CPacket::Alloc();
	SetSCPacket_RES_LOGIN(sendPacket, status, accountNo);
	ReqSendUnicast(sendPacket, user->_sessionID);

	MoveGroup(user->_sessionID, _pEchoGroup);
}

inline void CLoginGroup::GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, char*& sessionKey, int& version)
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
	*packet >> version;
}

inline void CLoginGroup::SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo)
{
	*packet << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
	*packet << status;
	*packet << accountNo;
}

BYTE CLoginGroup::LoginCheck(CLoginUser* user)
{
	return 1;
}
