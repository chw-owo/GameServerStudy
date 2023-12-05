#include <iostream>
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

int main()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	cpp_redis::client client;

	client.connect();

	client.set("hello", "42");
	client.get("hello", [](cpp_redis::reply& reply) {
		std::cout << reply << std::endl;
		});
	client.sync_commit();
}
