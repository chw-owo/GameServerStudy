#pragma once

#define LANSERVER
// #define NETSERVER

#ifdef LANSERVER
#pragma pack (push, 1)
struct stHeader
{
	unsigned short	Len;			// 메시지 길이
};
#pragma pack (pop)
#define dfHEADER_LEN				sizeof(stHeader)
#endif

#ifdef NETSERVER
#pragma pack (push, 1)
struct stHeader
{
	unsigned short	Len;			// 메시지 길이
};
#pragma pack (pop)
#define dfHEADER_LEN				sizeof(stHeader)
#endif

#define dfPACKET_DEFAULT			0
#define dfSESSION_MAX				32768

#define dfWSARECVBUF_CNT			2
#define dfWSASENDBUF_CNT			100

#define dfRBUFFER_DEF_SIZE			32768
#define dfRBUFFER_MAX_SIZE			100000

#define dfSPACKET_DEF_SIZE			1024
#define dfSPACKET_MAX_SIZE			4096

#define dfERRMSG_MAX				256