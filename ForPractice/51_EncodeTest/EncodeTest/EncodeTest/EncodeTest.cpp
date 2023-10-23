#include <stdio.h>
#include <windows.h>

struct stPacket
{
	unsigned char _code;
	unsigned short _len;
	unsigned char _randKey;
	unsigned char _checkSum;
	unsigned char _payload[55];
};

void Encode(stPacket& packet, char code, char fixedKey, char randKey, char payload[55])
{
	packet._code = code;
	packet._len = strlen(payload);
	packet._randKey = randKey;

	unsigned int checkSum = 0;
	for (int i = 0; i < strlen(payload); i++)
	{
		checkSum += payload[i];
	}
	packet._checkSum = checkSum % 256;
	memcpy_s(packet._payload, strlen(payload), payload, strlen(payload));

	char d;
	char p = 0;
	char e = 0;

	for (int i = 0; i <= strlen(payload); i++)
	{
		d = packet._payload[i];
		p = d ^ (p + randKey + 1 + i);
		e = p ^ (e + fixedKey + 1 + i);
		packet._payload[i] = e;
	}
}

int main()
{
	char payload[55] = "aaaaaaaaaabbbbbbbbbbcccccccccc1234567890abcdefghijklmn";
	for (int i = 0; i < 55; i++) ::printf("%02x ", payload[i]);

	stPacket packet;
	Encode(packet, 0x77, 0xa9, 0x31, payload);
	::printf("\n\n");

	for (int i = 0; i < 55; i++) ::printf("%02x ", packet._payload[i]);
}

