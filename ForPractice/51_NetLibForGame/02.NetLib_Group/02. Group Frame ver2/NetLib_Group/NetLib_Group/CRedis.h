#pragma once
#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "cpp_redis.lib")

class CRedis
{
private:
	CRedis()
	{
		InitializeSRWLock(&_lock);

		AcquireSRWLockExclusive(&_lock);
		_redis = new cpp_redis::client;
		_redis->connect();
		ReleaseSRWLockExclusive(&_lock);
	}
	~CRedis() {};

public:
	static CRedis* GetInstance()
	{
		static CRedis _CRedis;
		return &_CRedis;
	}

public:
	cpp_redis::client* _redis;
	SRWLOCK _lock;

private:
	static CRedis _CRedis;
};
#pragma once
