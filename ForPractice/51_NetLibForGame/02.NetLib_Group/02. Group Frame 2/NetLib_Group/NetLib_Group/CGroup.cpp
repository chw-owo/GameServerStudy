#include "CGroup.h"
#include "CNetServer.h"
#include <tchar.h>

void CGroup::AcceptEnterSessions()
{
	while (_enterSessions.GetUseSize() > 0)
	{
		CSession* pSession = _enterSessions.Dequeue();
		_sessions.push_back(pSession);
		OnEnterGroup(pSession->GetID());
	}
}

void CGroup::RemoveLeaveSessions()
{
	while (_leaveSessions.GetUseSize() > 0)
	{
		CSession* pSession = _leaveSessions.Dequeue();
		vector<CSession*>::iterator it = _sessions.begin();
		for (; it != _sessions.end(); it++)
		{
			if (*it == pSession)
			{
				_sessions.erase(it);
				OnLeaveGroup(pSession->GetID());
				break;
			}
		}
	}
}

void CGroup::RemoveReleaseSessions()
{
	while (_releaseSessions.GetUseSize() > 0)
	{
		CSession* pSession = _releaseSessions.Dequeue();
		vector<CSession*>::iterator it = _sessions.begin();
		for (; it != _sessions.end(); it++)
		{
			if (*it == pSession)
			{
				_sessions.erase(it);
				OnReleaseClient(pSession->GetID());
				PostQueuedCompletionStatus(_pNet->_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_releaseOvl);
				break;
			}
		}
	}
}

bool CGroup::SkipForFixedFrame()
{
	static DWORD oldTick = GetTickCount64();
	if ((GetTickCount64() - oldTick) < (1000 / _fps))
		return true;
	oldTick += (1000 / _fps);
	return false;
}

unsigned int __stdcall CGroup::UpdateThread(void* arg)
{
	CGroup* pGroup = (CGroup*)arg;

	for (;;)
	{
		if (pGroup->GetAlive()) break;
		pGroup->AcceptEnterSessions();
		pGroup->NetworkUpdate();

		if (!pGroup->SkipForFixedFrame())
		{
			pGroup->Update();
			pGroup->OnUpdate();
		}

		pGroup->RemoveLeaveSessions();
		pGroup->RemoveReleaseSessions();
	}

	return 0;
}

void CGroup::NetworkUpdate()
{
	vector<CSession*>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		CSession* pSession = *it;

		if (pSession->_recvBytes > 0)
			HandleRecv(pSession);

		if (pSession->_sendBytes > 0)
			HandleSend(pSession);
	}
}

void CGroup::HandleRecv(CSession* pSession)
{
	int recvBytes = pSession->_recvBytes;
	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		Disconnect(pSession->GetID());
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();
	while (useSize > dfHEADER_LEN)
	{
		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, stErrMsg);
			return;
		}

		if (header._code != dfPACKET_CODE)
		{
			Disconnect(pSession->GetID());
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Packet Code\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_PACKETCODE, stErrMsg);
			return;
		}

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return;
		}

		CPacket* packet = CPacket::Alloc();
		packet->Clear();
		packet->AddUsageCount(1);
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._len);
		if (dequeueRet != header._len)
		{
			Disconnect(pSession->GetID());
			CPacket::Free(packet);
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_PACKETLEN, stErrMsg);
			return;
		}
		packet->MovePayloadWritePos(dequeueRet);

		if (packet->Decode(header) == false)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			CPacket::Free(packet);
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Checksum\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_DECODE, stErrMsg);
			return;
		}

		OnRecv(pSession->GetID(), packet);
		CPacket::Free(packet);

		useSize = pSession->_recvBuf.GetUseSize();
	}

	pSession->_recvBytes = 0;
	_pNet->RecvPost(pSession);
}

void CGroup::HandleSend(CSession* pSession)
{
	int sendBytes = pSession->_sendBytes;
	for (int i = 0; i < pSession->_sendCount; i++)
	{
		CPacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		CPacket::Free(packet);
	}

	OnSend(pSession->GetID(), sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	pSession->_sendBytes = 0;
	_pNet->SendPost(pSession);
}

bool CGroup::Disconnect(unsigned __int64 sessionID)
{
	_pNet->Disconnect(sessionID);
}

bool CGroup::SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect)
{
	_pNet->SendPacket(sessionID, packet, disconnect);
}

void CGroup::MoveGroup(unsigned __int64 sessionID, CGroup* pGroup)
{
	_pNet->MoveGroup(sessionID, pGroup);
}
