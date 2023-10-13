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

// ================================================================================



// <Network Terminate> ============================================================

#define ERR_ALREADY_TERMINATE			10
#define ERR_TEMPSOCK_INVALID			11
#define ERR_TEMPSOCK_CONNECT			12

// ================================================================================



// <Network> ================================================================

#define ERR_ACCEPT						20
#define ERR_GQCS_RET0					21
#define ERR_RECV						22
#define ERR_SEND						23

// ================================================================================



// <Buffer> ==================================================================

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
