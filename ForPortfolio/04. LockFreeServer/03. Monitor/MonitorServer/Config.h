#pragma once

#define LANSERVER
#define NETSERVER

#ifdef LANSERVER
#pragma pack (push, 1)
struct stLanHeader
{
	unsigned short	_len;
};
#pragma pack (pop)
#define dfLANHEADER_LEN				sizeof(stLanHeader)
#endif

#ifdef NETSERVER

#define dfPACKET_CODE		109
#define dfPACKET_KEY		30

#pragma pack (push, 1)
struct stNetHeader
{
	unsigned char _code = dfPACKET_CODE;
	unsigned short _len;
	unsigned char _randKey;
	unsigned char _checkSum;
};
#pragma pack (pop)
#define dfNETHEADER_LEN				sizeof(stNetHeader)
#endif

#define dfSESSION_MAX				5
#define dfPACKET_DEF				100
#define dfRCV_PACKET_DEF			100
#define dfJOB_DEF					100

#define dfJOB_QUEUE_CNT				1
#define dfWSARECVBUF_CNT			1
#define dfWSASENDBUF_CNT			100

#define dfRBUFFER_DEF_SIZE			1024
#define dfRBUFFER_MAX_SIZE			16384

#define dfSPACKET_DEF_SIZE			256
#define dfSPACKET_MAX_SIZE			2048

#define dfERR_MAX					256