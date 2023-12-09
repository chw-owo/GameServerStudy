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

void CGroup::RemoveAllSessions()
{
	vector<CSession*>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		MoveGroup((*it)->GetID(), nullptr);
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
	pGroup->Initialize();
	pGroup->OnInitialize();

	while(pGroup->GetAlive())
	{
		pGroup->AcceptEnterSessions();
		pGroup->NetworkUpdate();
		pGroup->RemoveLeaveSessions();
		pGroup->RemoveReleaseSessions();

		if (!pGroup->SkipForFixedFrame())
		{
			pGroup->Update();
			pGroup->OnUpdate();
		}
	}

	pGroup->RemoveAllSessions();
	pGroup->Terminate();
	pGroup->OnTerminate();
	return 0;
}

void CGroup::NetworkUpdate()
{
	vector<CSession*>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		CSession* pSession = *it;

		while (pSession->_OnRecvQ.GetUseSize() > 0)
		{
			if (pSession->_pGroup != this) break;
			CPacket* packet = pSession->_OnRecvQ.Dequeue();
			OnRecv(pSession->GetID(), packet);
			CPacket::Free(packet);
		}

		while (pSession->_OnSendQ.GetUseSize() > 0)
		{
			if (pSession->_pGroup != this) break;
			OnSend(pSession->GetID(), pSession->_OnSendQ.Dequeue());
		}
	}
}

bool CGroup::Disconnect(unsigned __int64 sessionID)
{
	::printf("%llx (%d): CGroup::%s\n", sessionID, GetCurrentThreadId(), __func__);
	return _pNet->Disconnect(sessionID);
}

bool CGroup::SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect)
{
	::printf("%llx (%d): CGroup::%s\n", sessionID, GetCurrentThreadId(), __func__);
	return _pNet->SendPacket(sessionID, packet, disconnect);
}

void CGroup::MoveGroup(unsigned __int64 sessionID, CGroup* pGroup)
{
	::printf("%llx (%d): CGroup::%s\n", sessionID, GetCurrentThreadId(), __func__);
	_pNet->MoveGroup(sessionID, pGroup);
}
