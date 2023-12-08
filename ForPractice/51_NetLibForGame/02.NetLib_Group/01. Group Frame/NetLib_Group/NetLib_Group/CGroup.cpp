#include "CGroup.h"
#include "CNetServer.h"

void CGroup::NetworkUpdate()
{

}

void CGroup::AcceptEnterSessions()
{
	while (_enterSessions.GetUseSize() > 0)
	{
		CSession* pSession = _enterSessions.Dequeue();
		_sessions.push_back(pSession);
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
				break;
			}
		}
	}
}

bool CGroup::SkipForFixedFrame()
{
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
	}

	return 0;
}

bool CGroup::HandleRecv(CSession* pSession, int recvBytes)
{
	return false;
}

bool CGroup::HandleSend(CSession* pSession, int sendBytes)
{
	return false;
}

bool CGroup::RecvPost(CSession* pSession)
{
	return false;
}

bool CGroup::SendPost(CSession* pSession)
{
	return false;
}

void CGroup::HandleRelease(CSession* pSession)
{
}

bool CGroup::Disconnect(CSession* pSession)
{
	return false;
}

bool CGroup::SendPacket(CSession* pSession, CPacket* packet, bool disconnect)
{
	return false;
}

void CGroup::MoveGroup(CSession* pSession, CGroup* pGroup)
{
	CGroup* pCurGroup = pSession->_group;
	InterlockedExchangePointer((PVOID*)pSession->_group, pGroup);

	// Insert Session to New Group
	if (pCurGroup == nullptr)
		PostQueuedCompletionStatus(_pNet->_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_enterOvl);
	else
		pGroup->_enterSessions.Enqueue(pSession);

	// Remove Session from Cur Group
	_leaveSessions.Enqueue(pSession);
	
}
