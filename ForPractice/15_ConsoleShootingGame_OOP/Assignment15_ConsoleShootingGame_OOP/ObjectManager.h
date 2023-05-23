#pragma once
#include "List.cpp"
#include "BaseObject.h"

class CObjectManager
{
public:
	static CObjectManager* GetInstance();

	template <typename T, typename... Types>
	T* CreateObject(T* pObjectType, Types... args)
	{
		T* pObject = new T(args...);
		_ObjectList.push_back(pObject);
		return pObject;
	}

	template <typename T>
	void DestroyObject(T* object)
	{
		_ObjectList.remove(object);
		delete(object);
	}

	void DestroyAllObject()
	{
		CBaseObject* pObject;
		while (!_ObjectList.empty())
		{
			pObject = _ObjectList.pop_back();
			delete(pObject);
		}
	}

	void Update();
	void Render();

private:
	CObjectManager();
	~CObjectManager();

private:
	static CObjectManager _ObjectMgr;
	CList<CBaseObject*> _ObjectList;
};
