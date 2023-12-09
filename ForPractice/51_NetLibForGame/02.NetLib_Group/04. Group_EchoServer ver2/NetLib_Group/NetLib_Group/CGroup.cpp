#include "CGroup.h"
#include "CNetServer.h"

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
		// TO-DO: 아 Release Flag에서 걸려서 Release가 안되겠구나!!!!!!

		CSession* pSession = _pNet->AcquireSessionUsage((*it), __LINE__);
		if (pSession == nullptr) 
		{
			it++;
			continue;
		}

		EnterCriticalSection(&pSession->_groupLock);

		if (pSession->_pGroup != this)
		{
			OnLeaveGroup(pSession->GetID());
			it = _sessions.erase(it);
		}
		else if (pSession->_validFlag._releaseFlag == 1)
		{
			OnReleaseClient(pSession->GetID());
			PostQueuedCompletionStatus(_pNet->_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_releaseOvl);
			it = _sessions.erase(it);
		}
		else
		{
			while (pSession->_OnRecvQ.GetUseSize() > 0)
			{
				if (pSession->_pGroup != this)
					break;

				CPacket* packet = pSession->_OnRecvQ.Dequeue();
				OnRecv(pSession->GetID(), packet);
				CPacket::Free(packet);
			}

			while (pSession->_OnSendQ.GetUseSize() > 0)
			{
				if (pSession->_pGroup != this)
					break;

				OnSend(pSession->GetID(), pSession->_OnSendQ.Dequeue());
			}

			it++;
		}

		LeaveCriticalSection(&pSession->_groupLock);
		_pNet->ReleaseSessionUsage(pSession, __LINE__);
	}
}

void CGroup::AcceptEnterSessions()
{
	while (_enterSessions.GetUseSize() > 0)
	{
		unsigned __int64 sessionID = _enterSessions.Dequeue();
		_sessions.push_back(sessionID);
		OnEnterGroup(sessionID);
	}
}

void CGroup::RemoveAllSessions()
{
	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		MoveGroup((*it), nullptr);
	}
}
