#include "CGroup.h"
#include "CNetServer.h"
#include <tchar.h>

bool CGroup::Disconnect(unsigned __int64 sessionID)
{
	return _pNet->Disconnect(sessionID);
}

bool CGroup::SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect)
{
	return _pNet->SendPacket(sessionID, packet, disconnect);
}

void CGroup::MoveGroup(unsigned __int64 sessionID, CGroup* pGroup)
{
	_pNet->MoveGroup(sessionID, pGroup);
}

unsigned int __stdcall CGroup::UpdateThread(void* arg)
{
	CGroup* pGroup = (CGroup*)arg;
	pGroup->Initialize();
	pGroup->OnInitialize();

	while (pGroup->GetAlive())
	{
		pGroup->AcceptEnterSessions();
		pGroup->NetworkUpdate();

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

bool CGroup::SkipForFixedFrame()
{
	static DWORD oldTick = GetTickCount64();
	if ((GetTickCount64() - oldTick) < (1000 / _fps))
		return true;
	oldTick += (1000 / _fps);
	return false;
}

void CGroup::NetworkUpdate()
{
	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it != _sessions.end();)
	{
		unsigned __int64 sessionID = *it;
		CSession* pSession = _pNet->AcquireSessionUsage(sessionID);
		if (pSession == nullptr)
		{
			OnLeaveGroup(sessionID);
			it = _sessions.erase(it);
			_leaveCnt++;
			continue;
		}
		
		EnterCriticalSection(&pSession->_groupLock);
		if (pSession->_pGroup != this)
		{
			OnLeaveGroup(sessionID);
			it = _sessions.erase(it);
			_leaveCnt++;
		}
		else
		{
			while (pSession->_OnRecvQ.GetUseSize() > 0)
			{
				if (pSession->_pGroup != this) break;
				CPacket* packet = pSession->_OnRecvQ.Dequeue();
				OnRecv(sessionID, packet);
				CPacket::Free(packet);
			}

			while (pSession->_OnSendQ.GetUseSize() > 0)
			{
				if (pSession->_pGroup != this) break;
				OnSend(sessionID, pSession->_OnSendQ.Dequeue());
			}

			it++;
		}

		LeaveCriticalSection(&pSession->_groupLock);
		_pNet->ReleaseSessionUsage(pSession);
	}
}

void CGroup::AcceptEnterSessions()
{
	while (_enterSessions.GetUseSize() > 0)
	{
		unsigned __int64 sessionID = _enterSessions.Dequeue();
		_sessions.push_back(sessionID);
		OnEnterGroup(sessionID);
		_enterCnt++;
	}
}

void CGroup::RemoveAllSessions()
{
	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		MoveGroup((*it), nullptr);
		_leaveCnt++;
	}
}
