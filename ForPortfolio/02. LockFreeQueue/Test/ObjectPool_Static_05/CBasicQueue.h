#pragma once

#define dfTEST_TOTAL_CNT 2500000

template<typename T>
class CBasicQueue
{
public:
	void Enqueue(T value) 
	{						
		_queue[_rear] = value;
		_rear++;
	}

	T Dequeue() 
	{	
		if (_front == _rear) return 0;

		T value = _queue[_front];
		_front++;
		return value;	
	}

private:
	T _queue[dfTEST_TOTAL_CNT];
	int _rear = 0;
	int _front = 0;
};