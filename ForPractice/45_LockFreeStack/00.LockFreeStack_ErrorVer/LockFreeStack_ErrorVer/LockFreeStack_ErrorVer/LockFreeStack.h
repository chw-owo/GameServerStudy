#pragma once
#include <windows.h>

template<typename T>
class LockFreeStack
{
private:
	struct Node
	{
		T _data;
		Node* _next;
	};

private:
	Node* _pTop = nullptr;

public:
	void Push(T data)
	{
		Node* pNew = new Node;
		pNew->_data = data;

		for(;;)
		{
			Node* pTopLocal = _pTop;
			pNew->_next = pTopLocal;

			if (InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNew, (LONG64)pTopLocal))
			{
				break;
			}
		} 
	}

	void Pop()
	{
		for(;;)
		{
			Node* pTopLocal = _pTop;
			Node* pPop = pTopLocal;
			Node* pNext = pPop->_next;

			if (InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNext, (LONG64)pTopLocal))
			{
				delete pPop;	
				break;
			}
		} 
	}
};



