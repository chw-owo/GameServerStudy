#include "Packet.h"
#include <stdio.h>

int main()
{
	CPacket cPacket;

	cPacket << (int)40030 << (BYTE)1 << (float)1.4;

	int iValue;
	BYTE byValue;
	float fValue;

	cPacket >> iValue >> byValue >> fValue;		

	printf("%d\n", iValue);
	printf("%d\n", byValue);
	printf("%f\n", fValue);
}
