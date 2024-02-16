#pragma once
//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__

// Network Info
#define dfSERVER_IP				L"0.0.0.0"
#define dfLOGIN_PORT			12049
#define dfCHAT_PORT				12050
#define dfGAME_PORT				0
#define dfSERVER_DISCONNECT		false
#define dfTHREAD_MAX			32
#define dfSEND_TIME				0

// Use in Login Server
#define dfUSER_MAX				6000
#define dfACCOUNTNO_LEN			20
#define dfID_LEN				20
#define dfNICKNAME_LEN			20
#define dfSESSIONKEY_LEN		64
#define dfIP_LEN				16
#define dfQUERY_MAX				50
#define dfREDIS_TIMEOUT			10

// Use in Chat Server
#define dfPLAYER_MAX			6000
#define dfJOB_DEF				10000
#define dfTIMEOUT				40000
#define dfSECTOR_CNT_Y			50
#define dfSECTOR_CNT_X			50
#define dfMONITOR_TEXT_LEN		2048
#define dfMSG_MAX				1024

// Monitor Client
#define dfMONIOTOR_IP			L"127.0.0.1"
#define dfMONIOTOR_PORT			12001

enum en_PACKET_TYPE
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER = 0,

	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];		// ������ū
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null ����
	//		WCHAR	Nickname[20]				// null ����
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_MESSAGE,

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
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,






	//------------------------------------------------------
	// Login Server
	//------------------------------------------------------
	en_PACKET_CS_LOGIN_SERVER = 100,

	//------------------------------------------------------------
	// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_REQ_LOGIN,

	//------------------------------------------------------------
	// �α��� �������� Ŭ���̾�Ʈ�� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (���ǿ���) / 1 (����) ...  �ϴ� defines ���
	//
	//		WCHAR	ID[20]				// ����� ID		. null ����
	//		WCHAR	Nickname[20]		// ����� �г���	. null ����
	//
	//		WCHAR	GameServerIP[16]	// ���Ӵ�� ����,ä�� ���� ����
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_RES_LOGIN,



	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	en_PACKET_CS_GAME_SERVER = 1000,


	//------------------------------------------------------
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


enum en_PACKET_CS_LOGIN_RES_LOGIN
{
	dfLOGIN_STATUS_NONE = -1,		// ����������
	dfLOGIN_STATUS_FAIL = 0,		// ���ǿ���
	dfLOGIN_STATUS_OK = 1,		// ����
	dfLOGIN_STATUS_GAME = 2,		// ������
	dfLOGIN_STATUS_ACCOUNT_MISS = 3,		// account ���̺� AccountNo ����
	dfLOGIN_STATUS_SESSION_MISS = 4,		// Session ���̺� AccountNo ����
	dfLOGIN_STATUS_STATUS_MISS = 5,		// Status ���̺� AccountNo ����
	dfLOGIN_STATUS_NOSERVER = 6,		// �������� ������ ����.
};


enum en_PACKET_CS_GAME_RES_LOGIN
{
	dfGAME_LOGIN_FAIL = 0,		// ����Ű ���� �Ǵ� Account ���̺���� ����
	dfGAME_LOGIN_OK = 1,		// ����
	dfGAME_LOGIN_NOCHARACTER = 2,		// ���� / ĳ���� ���� > ĳ���� ����ȭ������ ��ȯ. 
	dfGAME_LOGIN_VERSION_MISS = 3,		// ����,Ŭ�� ���� �ٸ�
};



enum en_PACKET_SS_LOGINSERVER_LOGIN
{
	dfSERVER_TYPE_GAME = 1,
	dfSERVER_TYPE_CHAT = 2,
	dfSERVER_TYPE_MONITOR = 3,
};

enum en_PACKET_SS_HEARTBEAT
{
	dfTHREAD_TYPE_WORKER = 1,
	dfTHREAD_TYPE_DB = 2,
	dfTHREAD_TYPE_GAME = 3,
};

// en_PACKET_SS_MONITOR_LOGIN
enum en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL
{
	dfMONITOR_SERVER_TYPE_LOGIN = 1,
	dfMONITOR_SERVER_TYPE_GAME = 2,
	dfMONITOR_SERVER_TYPE_CHAT = 3,
	dfMONITOR_SERVER_TYPE_AGENT = 4,

	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// ���� �������� (���Ӽ��� ����)
	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// ���� ���μ��� ��������
	dfMONITOR_SERVER_CONTROL_RUN = 3,		// ���� ���μ��� ���� & ����
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