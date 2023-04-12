#pragma once
template <typename T>
class List
{
public:
	struct Node
	{
		T _data;
		Node* _prev;
		Node* _next;
	};

	class iterator
	{
		friend class List;
	private:
		Node* _node;
	public:
		iterator(Node* node = nullptr) : _node(node)
		{

		}
		const iterator operator ++(int)
		{
			iterator origin = *this;
			_node = _node->_next;
			return origin;
		}
		iterator& operator++()
		{
			_node = _node->_next;
			return *this;
		}
		const iterator operator --(int)
		{
			iterator origin = *this;
			_node = _node->_prev;
			return origin;
		}
		iterator& operator--()
		{
			_node = _node->_prev;
			return *this;
		}
		T& operator *()
		{
			return _node->_data;
		}
		bool operator == (const iterator& other)
		{
			return (_node == other._node);
		}
		bool operator !=(const iterator& other)
		{
			return (_node != other._node);
		}
	};

public:
	List()
	{
		_head._prev = nullptr;
		_head._next = &_tail;
		_tail._prev = &_head;
		_tail._next = nullptr;
	}
	~List()
	{
		clear();
	}

	iterator begin()
	{ 
		return iterator(_head._next);
	}
	iterator end()
	{
		return iterator(&_tail);
	}

	void push_front(T data)
	{
		Node* node = new Node;
		node->_data = data;
		node->_prev = &_head;
		node->_next = _head._next;
		(_head._next)->_prev = node;
		_head._next = node;
		_size++;
	}
	void push_back(T data)
	{
		Node* node = new Node;
		node->_data = data;
		node->_prev = _tail._prev;
		node->_next = &_tail;
		(_tail._prev)->_next = node;
		_tail._prev = node;
		_size++;
	}
	void pop_front()
	{
		Node* nodePtr = _head._next;
		_head._next = nodePtr->_next;
		(_head._next)->_prev = &_head;
		delete(nodePtr);
		_size--;
	}
	void pop_back()
	{
		Node* nodePtr = _tail._prev;
		_tail._prev = nodePtr->_prev;
		(_tail._prev)->_next = &_tail;
		delete(nodePtr);
		_size--;
	}
	void clear()
	{
		Node* nodePtr = _head._next;
		Node* prevNodePtr;

		while (nodePtr != &_tail)
		{
			prevNodePtr = nodePtr;
			nodePtr = prevNodePtr->_next;
			delete(prevNodePtr);
		}

		_head._next = &_tail;
		_tail._prev = &_head;
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
		Node* nodePtr = iter._node->_next;
		(iter._node->_next)->_prev = iter._node->_prev;
		(iter._node->_prev)->_next = iter._node->_next;
		delete(iter._node);
		_size--;

		iter._node = nodePtr;
		return iter;
	}
	void remove(T Data)
	{
		List<T>::iterator iter;
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
