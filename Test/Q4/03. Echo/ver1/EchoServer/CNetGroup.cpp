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

	if (pGroup->_fps > 0)
	{
		while (pGroup->GetAlive())
		{
			pGroup->NetworkUpdate();
			if (!pGroup->SkipForFixedFrame())
			{
				pGroup->Update();
				pGroup->OnUpdate();
			}
		}
	}
	else
	{
		while (pGroup->GetAlive())
		{
			pGroup->NetworkUpdate();
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
	if (_fps == 0)
		WaitOnAddress(&_signal, &_undesired, sizeof(long), INFINITE);
	else
		WaitOnAddress(&_signal, &_undesired, sizeof(long), (1000 / _fps));

	while (_enterSessions.GetUseSize() > 0)
	{
		unsigned __int64 sessionID = _enterSessions.Dequeue();
		_sessions.push_back(sessionID);
		OnEnterGroup(sessionID);
		InterlockedIncrement(&_enterCnt);
		InterlockedDecrement(&_signal);
	}

	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it != _sessions.end();)
	{
		unsigned __int64 sessionID = *it;
		CSession* pSession = _pNet->AcquireSessionUsage(sessionID);
		if (pSession == nullptr)
		{
			OnLeaveGroup(sessionID);
			it = _sessions.erase(it);
			InterlockedIncrement(&_leaveCnt);
			InterlockedDecrement(&_signal);
			continue;
		}

		EnterCriticalSection(&pSession->_groupLock);
		if (pSession->_pGroup != this)
		{
			OnLeaveGroup(sessionID);
			it = _sessions.erase(it);
			InterlockedIncrement(&_leaveCnt);
			InterlockedDecrement(&_signal);
		}
		else
		{
			while (pSession->_OnRecvQ.GetUseSize() > 0)
			{
				if (pSession->_pGroup != this) break;
				CPacket* packet = pSession->_OnRecvQ.Dequeue();
				OnRecv(sessionID, packet);
				CPacket::Free(packet);
				InterlockedDecrement(&_signal);
			}
			it++;
		}

		LeaveCriticalSection(&pSession->_groupLock);
		_pNet->ReleaseSessionUsage(pSession);
	}

	while (_OnSendQ.GetUseSize() > 0)
	{
		OnSend(_OnSendQ.Dequeue());
		InterlockedDecrement(&_signal);
	}
}

void CGroup::RemoveAllSessions()
{
	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		MoveGroup((*it), nullptr);
		InterlockedIncrement(&_leaveCnt);
	}
}
