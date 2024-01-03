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

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32

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

#define dfSESSION_MAX				20000
#define dfADDRESS_LEN				50

#define dfPACKET_DEF				40000
#define dfRCV_PACKET_DEF			20000
#define dfJOB_DEF					20000

#define dfJOB_QUEUE_CNT				1
#define dfWSARECVBUF_CNT			1
#define dfWSASENDBUF_CNT			200

#define dfSPACKET_DEF_SIZE			256
#define dfSPACKET_MAX_SIZE			2048

#define dfERR_MAX					256