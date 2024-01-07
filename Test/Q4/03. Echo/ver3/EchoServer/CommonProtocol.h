#pragma once

// Network Info
#define dfSERVER_IP				L"0.0.0.0"
#define dfLOGIN_PORT			12077
#define dfECHO_PORT				12078
#define dfSEND_TIME				30
#define dfSERVER_DISCONNECT		false
#define dfTHREAD_MAX			32
#define dfUSER_MAX				15000
#define dfTIMEOUT				40000

// Login Server
#define dfSESSIONKEY_LEN		64

// Monitor Client
#define dfMONIOTOR_IP			L"127.0.0.1"
#define dfMONIOTOR_PORT			12001
#define dfMONITOR_TEXT_LEN		1024

enum en_PACKET_TYPE
{
	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	en_PACKET_CS_GAME_SERVER = 1000,

	//------------------------------------------------------------
	// �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//
	//		int		Version			// 1 
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_GAME_REQ_LOGIN,

	//------------------------------------------------------------
	// �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status (0: ���� / 1: ����)
	//		INT64	AccountNo
	//	}
	//
	//	���� ���̴� ������ �������� �Ǵ��ϰ� ����
	//	Status ����� �����Ѵٴ� �̾߱�
	//
	//  en_PACKET_CS_GAME_RES_LOGIN define �� ���.
	//------------------------------------------------------------
	en_PACKET_CS_GAME_RES_LOGIN,



	//------------------------------------------------------------
	// �׽�Ʈ�� ���� ��û
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------	
	en_PACKET_CS_GAME_REQ_ECHO = 5000,

	//------------------------------------------------------------
	// �׽�Ʈ�� ���� ���� (REQ �� �״�� ������)
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_GAME_RES_ECHO,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
	en_PACKET_CS_GAME_REQ_HEARTBEAT,


	//------------------------------------------------------
	// Monitor Server Protocol
	//------------------------------------------------------


	////////////////////////////////////////////////////////
	//
	//   MonitorServer & MoniterTool Protocol / ������ ���� ����.
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Monitor Server  Protocol
	//------------------------------------------------------


	en_PACKET_SS_MONITOR = 20000,
	//------------------------------------------------------
	// Server -> Monitor Protocol
	//------------------------------------------------------
	//------------------------------------------------------------
	// LoginServer, GameServer , ChatServer  �� ����͸� ������ �α��� ��
	//
	// 
	//	{
	//		WORD	Type
	//
	//		int		ServerNo		//  �� �������� ���� ��ȣ�� �ο��Ͽ� ���
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_LOGIN,

	//------------------------------------------------------------
	// ������ ����͸������� ������ ����
	// �� ������ �ڽ��� ����͸����� ��ġ�� 1�ʸ��� ����͸� ������ ����.
	//
	// ������ �ٿ� �� ��Ÿ ������ ����͸� �����Ͱ� ���޵��� ���ҋ��� ����Ͽ� TimeStamp �� �����Ѵ�.
	// �̴� ����͸� Ŭ���̾�Ʈ���� ���,�� ����Ѵ�.
	// 
	//	{
	//		WORD	Type
	//
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_DATA_UPDATE,


	en_PACKET_CS_MONITOR = 25000,
	//------------------------------------------------------
	// Monitor -> Monitor Tool Protocol  (Client <-> Server ��������)
	//------------------------------------------------------
	//------------------------------------------------------------
	// ����͸� Ŭ���̾�Ʈ(��) �� ����͸� ������ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		char	LoginSessionKey[32]		// �α��� ���� Ű. (�̴� ����͸� ������ ���������� ����)
	//										// �� ����͸� ���� ���� Ű�� ������ ���;� ��
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,

	//------------------------------------------------------------
	// ����͸� Ŭ���̾�Ʈ(��) ����͸� ������ �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status					// �α��� ��� 0 / 1 / 2 ... �ϴ� Define
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_MONITOR_TOOL_RES_LOGIN,

	//------------------------------------------------------------
	// ����͸� ������ ����͸� Ŭ���̾�Ʈ(��) ���� ����͸� ������ ����
	// 
	// ���� ����͸� ����� ��� ���̹Ƿ�, ����͸� ������ ��� ����͸� Ŭ���̾�Ʈ����
	// �����Ǵ� ��� �����͸� �ٷ� ���۽��� �ش�.
	// 
	//
	// �����͸� �����ϱ� ���ؼ��� �ʴ����� ��� �����͸� ��� 30~40���� ����͸� �����͸� �ϳ��� ��Ŷ���� ����°�
	// ������  �������� ������ ������ �����Ƿ� �׳� ������ ����͸� �����͸� ���������� ����ó�� �Ѵ�.
	//
	//	{
	//		WORD	Type
	//		
	//		BYTE	ServerNo				// ���� No
	//		BYTE	DataType				// ����͸� ������ Type �ϴ� Define ��.
	//		int		DataValue				// �ش� ������ ��ġ.
	//		int		TimeStamp				// �ش� �����͸� ���� �ð� TIMESTAMP  (time() �Լ�)
	//										// ���� time �Լ��� time_t Ÿ�Ժ����̳� 64bit �� ���񽺷����
	//										// int �� ĳ�����Ͽ� ����. �׷��� 2038�� ������ ��밡��
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_MONITOR_TOOL_DATA_UPDATE,


};


enum en_PACKET_SS_MONITOR_DATA_UPDATE
{
	en_MONITOR_DATA_TYPE_LOGIN_SERVER_RUN = 1,			// �α��μ��� ���࿩�� ON / OFF
	en_MONITOR_DATA_TYPE_LOGIN_SERVER_CPU = 2,			// �α��μ��� CPU ����
	en_MONITOR_DATA_TYPE_LOGIN_SERVER_MEM = 3,			// �α��μ��� �޸� ��� MByte
	en_MONITOR_DATA_TYPE_LOGIN_SESSION = 4,				// �α��μ��� ���� �� (���ؼ� ��)
	en_MONITOR_DATA_TYPE_LOGIN_AUTH_TPS = 5,			// �α��μ��� ���� ó�� �ʴ� Ƚ��
	en_MONITOR_DATA_TYPE_LOGIN_PACKET_POOL = 6,			// �α��μ��� ��ŶǮ ��뷮

	en_MONITOR_DATA_TYPE_GAME_SERVER_RUN = 10,			// GameServer ���� ���� ON / OFF
	en_MONITOR_DATA_TYPE_GAME_SERVER_CPU = 11,			// GameServer CPU ����
	en_MONITOR_DATA_TYPE_GAME_SERVER_MEM = 12,			// GameServer �޸� ��� MByte
	en_MONITOR_DATA_TYPE_GAME_SESSION = 13,				// ���Ӽ��� ���� �� (���ؼ� ��)
	en_MONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 14,			// ���Ӽ��� AUTH MODE �÷��̾� ��
	en_MONITOR_DATA_TYPE_GAME_GAME_PLAYER = 15,			// ���Ӽ��� GAME MODE �÷��̾� ��
	en_MONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 16,			// ���Ӽ��� Accept ó�� �ʴ� Ƚ��
	en_MONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS = 17,		// ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
	en_MONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 18,		// ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
	en_MONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 19,		// ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
	en_MONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 20,		// ���Ӽ��� DB ���� �޽��� ť ���� (���� ��)
	en_MONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 21,		// ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
	en_MONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 22,		// ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
	en_MONITOR_DATA_TYPE_GAME_PACKET_POOL = 23,			// ���Ӽ��� ��ŶǮ ��뷮

	en_MONITOR_DATA_TYPE_CHAT_SERVER_RUN = 30,			// ä�ü��� ChatServer ���� ���� ON / OFF
	en_MONITOR_DATA_TYPE_CHAT_SERVER_CPU = 31,			// ä�ü��� ChatServer CPU ����
	en_MONITOR_DATA_TYPE_CHAT_SERVER_MEM = 32,			// ä�ü��� ChatServer �޸� ��� MByte
	en_MONITOR_DATA_TYPE_CHAT_SESSION = 33,				// ä�ü��� ���� �� (���ؼ� ��)
	en_MONITOR_DATA_TYPE_CHAT_PLAYER = 34,				// ä�ü��� �������� ����� �� (���� ������)
	en_MONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 35,			// ä�ü��� UPDATE ������ �ʴ� ó�� Ƚ��
	en_MONITOR_DATA_TYPE_CHAT_PACKET_POOL = 36,			// ä�ü��� ��ŶǮ ��뷮
	en_MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 37,		// ä�ü��� UPDATE MSG Ǯ ��뷮

	en_MONITOR_DATA_TYPE_MONITOR_CPU_TOTAL = 40,			// ������ǻ�� CPU ��ü ����
	en_MONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY = 41,		// ������ǻ�� �������� �޸� MByte
	en_MONITOR_DATA_TYPE_MONITOR_NETWORK_RECV = 42,			// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
	en_MONITOR_DATA_TYPE_MONITOR_NETWORK_SEND = 43,			// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
	en_MONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY = 44,		// ������ǻ�� ��밡�� �޸� MByte
};


enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
{
	en_MONITOR_TOOL_LOGIN_OK = 1,				// �α��� ����
	en_MONITOR_TOOL_LOGIN_ERR_NOSERVER = 2,		// �����̸� ���� (��Ī�̽�)
	en_MONITOR_TOOL_LOGIN_ERR_SESSIONKEY = 3,	// �α��� ����Ű ����
};

//#endif