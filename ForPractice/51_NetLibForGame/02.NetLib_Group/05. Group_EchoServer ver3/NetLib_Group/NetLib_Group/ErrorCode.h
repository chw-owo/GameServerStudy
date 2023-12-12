#pragma once

// <Network Initialize> =========================================================

// Network Setting Error
#define ERR_WSASTARTUP					0
#define ERR_LISTENSOCK_INVALID			1
#define ERR_SET_LINGER					2
#define ERR_SET_SNDBUF_0				3
#define ERR_LISTENSOCK_BIND				4
#define ERR_LISTEN						5

// Thread Setting Error
#define ERR_CREATE_IOCP					6
#define ERR_CREATE_ACCEPT_THREAD		7
#define ERR_CREATE_NETWORK_THREAD		8
#define ERR_CREATE_GROUP_THREAD			9

// ================================================================================

// <Network Terminate> ============================================================

#define ERR_ALREADY_TERMINATE			10
#define ERR_TEMPSOCK_INVALID			11
#define ERR_TEMPSOCK_CONNECT			12

// ================================================================================



// <Network> ======================================================================

#define ERR_ACCEPT						20
#define ERR_GQCS_RET0					21
#define ERR_RECV						22
#define ERR_SEND						23

// ================================================================================



// <Buffer In NetLib> =============================================================

// Serialize Packet
#define ERR_PACKET_PUT_HEADER			30

// Recv Buffer
#define ERR_RECVBUF_MOVEWRITEPOS		31
#define ERR_RECVBUF_MOVEREADPOS			32
#define ERR_RECVBUF_PEEK				33
#define ERR_RECVBUF_DEQ					34

// Send Buffer
#define ERR_SENDBUF_MOVEREADPOS			35
#define ERR_SENDBUF_ENQ					36

// ================================================================================



// <Buffer> =======================================================================

// Serialize Packet
#define ERR_PACKET						-1
#define ERR_RESIZE_OVER_MAX				0

#define ERR_MOVE_PAYLOAD_READ_UNDER		1
#define ERR_MOVE_PAYLOAD_READ_OVER		2
#define ERR_MOVE_PAYLOAD_WRITE_UNDER	3
#define ERR_MOVE_PAYLOAD_WRITE_OVER		4

#define ERR_MOVE_HEADER_READ_UNDER		5
#define ERR_MOVE_HEADER_READ_OVER		6
#define ERR_MOVE_HEADER_WRITE_UNDER		7
#define ERR_MOVE_HEADER_WRITE_OVER		8

#define ERR_GET_FLOAT_OVER				9
#define ERR_GET_DOUBLE_OVER				10
#define ERR_GET_CHAR_OVER				11
#define ERR_GET_BYTE_OVER				12
#define ERR_GET_WCHAR_OVER				13
#define ERR_GET_SHORT_OVER				14
#define ERR_GET_WORD_OVER				15
#define ERR_GET_DWORD_OVER				16
#define ERR_GET_INT_OVER				17
#define ERR_GET_INT64_OVER				18

#define ERR_GET_PAYLOAD_OVER			19
#define ERR_PEEK_PAYLOAD_OVER			20
#define ERR_GET_HEADER_OVER_MAX			21
#define ERR_GET_HEADER_OVER_EMPTY		22
#define ERR_PEEK_HEADER_OVER_MAX		23
#define ERR_PEEK_HEADER_OVER_EMPTY		24
#define ERR_PUT_HEADER_OVER				25

// Ring Buffer
#define ERR_RINGBUFFER						-1
#define ERR_RESIZE_OVER_MAX					0
#define ERR_RESIZE_UNDER_USE				1

#define ERR_PEEK_OVER						3
#define ERR_ENQ_OVER						4
#define ERR_ENQ_OVER_AFTER_RESIZE			5
#define ERR_DEQ_OVER						6
#define ERR_MOVE_READ_OVER					7
#define ERR_MOVE_WRITE_OVER					8
#define ERR_MOVE_WRITE_OVER_AFTER_RESIZE	9

// ================================================================================


// <Debug> ======================================================================

#define DEB_WRONG_PACKETCODE			100
#define DEB_WRONG_PACKETLEN				101
#define DEB_WRONG_DECODE				102
#define DEB_SESSION_MAX					103

// ================================================================================
