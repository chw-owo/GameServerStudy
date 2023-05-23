#include "ObjectManager.h"

CObjectManager::CObjectManager()
{

}

CObjectManager::~CObjectManager()
{

}

CObjectManager* CObjectManager::GetInstance()
{
	static CObjectManager _ObjectMgr;
	return &_ObjectMgr;
}

void CObjectManager::Update()
{
	CList<CBaseObject*>::iterator iter = _ObjectList.begin();

	for (; iter != _ObjectList.end(); ++iter)
	{
		// Update
		(*iter)->Update();

		// 충돌 체크
		CList<CBaseObject*>::iterator targetIter = _ObjectList.begin();
		for (; targetIter != _ObjectList.end(); ++targetIter)
		{
			if (!(*iter)->GetDead() && !(*targetIter)->GetDead() &&
				(*iter)->GetX() == (*targetIter)->GetX() &&
				(*iter)->GetY() == (*targetIter)->GetY())
			{
				(*iter)->OnCollision((*targetIter));
				(*targetIter)->OnCollision((*iter));
			}
		}
	}
}

void CObjectManager::Render()
{
	CList<CBaseObject*>::iterator iter = _ObjectList.begin();
	for (; iter != _ObjectList.end(); ++iter)
	{
		(*iter)->Render();
	}
}