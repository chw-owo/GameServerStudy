#pragma once
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

class CRedis
{
private:
	CRedis()
	{
		_redis = new cpp_redis::client;
		_redis->connect();
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

private: 
	static CRedis _CRedis;
};
