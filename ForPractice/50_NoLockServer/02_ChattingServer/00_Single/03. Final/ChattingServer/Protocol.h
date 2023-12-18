#pragma once
//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__

// ���� 50 x 50
#define dfSERVER_IP				L"0.0.0.0"
#define dfSERVER_PORT			12050

#define dfSECTOR_CNT_Y			50
#define dfSECTOR_CNT_X			50
#define dfPLAYER_MAX			20000
#define dfTIMEOUT				40000
#define dfMONITOR_TEXT_LEN		1024
#define dfJOB_DEF				10000

#define dfID_LEN				20
#define dfNICKNAME_LEN			20
#define dfSESSIONKEY_LEN		64

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
	en_PACKET_SS_MONITOR_DATA_UPDATE

};



enum en_PACKET_SS_MONITOR_DATA_UPDATE
{
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN = 1,		// �α��μ��� ���࿩�� ON / OFF
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU = 2,		// �α��μ��� CPU ����
	dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM = 3,		// �α��μ��� �޸� ��� MByte
	dfMONITOR_DATA_TYPE_LOGIN_SESSION = 4,			// �α��μ��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS = 5,			// �α��μ��� ���� ó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL = 6,		// �α��μ��� ��ŶǮ ��뷮


	dfMONITOR_DATA_TYPE_GAME_SERVER_RUN = 10,		// GameServer ���� ���� ON / OFF
	dfMONITOR_DATA_TYPE_GAME_SERVER_CPU = 11,		// GameServer CPU ����
	dfMONITOR_DATA_TYPE_GAME_SERVER_MEM = 12,		// GameServer �޸� ��� MByte
	dfMONITOR_DATA_TYPE_GAME_SESSION = 13,			// ���Ӽ��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 14,		// ���Ӽ��� AUTH MODE �÷��̾� ��
	dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER = 15,		// ���Ӽ��� GAME MODE �÷��̾� ��
	dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 16,		// ���Ӽ��� Accept ó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS = 17,	// ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 18,	// ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 19,		// ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 20,		// ���Ӽ��� DB ���� �޽��� ť ���� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 21,	// ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 22,	// ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
	dfMONITOR_DATA_TYPE_GAME_PACKET_POOL = 23,		// ���Ӽ��� ��ŶǮ ��뷮

	dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN = 30,		// ä�ü��� ChatServer ���� ���� ON / OFF
	dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU = 31,		// ä�ü��� ChatServer CPU ����
	dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM = 32,		// ä�ü��� ChatServer �޸� ��� MByte
	dfMONITOR_DATA_TYPE_CHAT_SESSION = 33,			// ä�ü��� ���� �� (���ؼ� ��)
	dfMONITOR_DATA_TYPE_CHAT_PLAYER = 34,			// ä�ü��� �������� ����� �� (���� ������)
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 35,		// ä�ü��� UPDATE ������ �ʴ� �ʸ� Ƚ��
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL = 36,		// ä�ü��� ��ŶǮ ��뷮
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 37,	// ä�ü��� UPDATE MSG Ǯ ��뷮


	dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL = 40,			// ������ǻ�� CPU ��ü ����
	dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY = 41,	// ������ǻ�� �������� �޸� MByte
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV = 42,		// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
	dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND = 43,		// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
	dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY = 44,	// ������ǻ�� ��밡�� �޸�
};

//#endif


