#pragma once
template <typename T>
class CList
{
public:
	struct Node
	{
		T _data;
		Node* _Prev;
		Node* _Next;
	};

	class iterator
	{
	private:
		Node* _node;
	public:
		friend class CList;
	public:
		iterator(Node* node = nullptr) : _node(node)
		{

		}
		iterator operator ++(int)
		{
			iterator origin = *this;
			this->_node = _node->_Next;
			return origin;
		}
		iterator& operator++()
		{
			this->_node = _node->_Next;
			return *this;
		}
		iterator operator --(int)
		{
			iterator origin = *this;
			this->_node = _node->_Prev;
			return origin;
		}
		iterator& operator--()
		{
			this->_node = _node->_Prev;
			return *this;
		}
		T& operator *()
		{
			return this->_node->_data;
		}
		bool operator ==(const iterator& other)
		{
			return (this->_node == other._node);
		}
		bool operator !=(const iterator& other)
		{
			return (this->_node != other._node);
		}
	};

public:
	CList()
	{
		_head._Prev = nullptr;
		_head._Next = &_tail;
		_tail._Prev = &_head;
		_tail._Next = nullptr;
	}
	~CList()
	{
		clear();
	}

	iterator begin()
	{
		return iterator(_head._Next);
	}
	iterator end()
	{
		return iterator(&_tail);
	}

	void push_front(T data)
	{
		Node* node = new Node;
		node->_data = data;
		node->_Prev = &_head;
		node->_Next = _head._Next;
		(_head._Next)->_Prev = node;
		_head._Next = node;
		_size++;
	}

	void push_back(T data)
	{
		Node* node = new Node;
		node->_data = data;
		node->_Prev = _tail._Prev;
		node->_Next = &_tail;
		(_tail._Prev)->_Next = node;
		_tail._Prev = node;
		_size++;
	}

	T pop_front()
	{
		Node* nodePtr = _head._Next;
		T data = nodePtr->_data;
		_head._Next = nodePtr->_Next;
		(_head._Next)->_Prev = &_head;
		delete(nodePtr);
		_size--;
		return data;
	}

	T pop_back()
	{
		Node* nodePtr = _tail._Prev;
		T data = nodePtr->_data;
		_tail._Prev = nodePtr->_Prev;
		(_tail._Prev)->_Next = &_tail;
		delete(nodePtr);
		_size--;
		return data;
	}

	void clear()
	{
		Node* nodePtr = _head._Next;
		Node* prevNodePtr;

		while (nodePtr != &_tail)
		{
			prevNodePtr = nodePtr;
			nodePtr = prevNodePtr->_Next;
			delete(prevNodePtr);
		}

		_head._Next = &_tail;
		_tail._Prev = &_head;
		_size = 0;
	}

	int size()
	{
		return _size;
	}

	bool empty()
	{
		return (_size == 0);
	}

	iterator erase(iterator iter)
	{
		Node* nodePtr = iter._node->_Next;
		(iter._node->_Next)->_Prev = iter._node->_Prev;
		(iter._node->_Prev)->_Next = iter._node->_Next;
		delete(iter._node);
		_size--;

		iter._node = nodePtr;
		return iter;
	}

	void remove(T Data)
	{
		CList<T>::iterator iter;
		for (iter = begin(); iter != end(); )
		{
			if ((*iter) == Data)
			{
				iter = erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

private:
	int _size = 0;
	Node _head;
	Node _tail;
};
