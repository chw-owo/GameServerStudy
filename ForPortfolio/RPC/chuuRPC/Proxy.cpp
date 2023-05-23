#include "Proxy.h"
#include "Network.h"
#include "SerializePacket.h"

Proxy::Proxy()
{
	_net = NetworkManager::GetInstance();
}

Proxy::~Proxy()
{

}

Proxy* Proxy::GetInstance()
{
	static Proxy _proxy;
	return &_proxy;
}

void Proxy::SC_CreateMyCharacter(Session* pSession, int ID, char direction, short x, short y, char hp)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_CREATE_MY_CHARACTER, ID, direction, x, y, hp);
	packet << ID << direction << x << y << hp;
	_net->EnqueueUnicast(packet.GetReadPtr(), packet.GetDataSize(), pSession);
}

void Proxy::SC_CreateOtherCharacter(TARGET target, Session* pSession, int ID, char direction, short x, short y, char hp)
{
	if (target == UNI)
	{
		SerializePacket packet;
		CreateHeader(&packet, df_SC_CREATE_OTHER_CHARACTER, ID, direction, x, y, hp);
		packet << ID << direction << x << y << hp;
		_net->EnqueueUnicast(packet.GetReadPtr(), packet.GetDataSize(), pSession);
	}
	else if (target == BROAD_EXP)
	{
		SerializePacket packet;
		CreateHeader(&packet, df_SC_CREATE_OTHER_CHARACTER, ID, direction, x, y, hp);
		packet << ID << direction << x << y << hp;
		_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize(), pSession);
	}
}

void Proxy::SC_DeleteCharacter(int ID)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_DELETE_CHARACTER, ID);
	packet << ID;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize());
}

void Proxy::SC_MoveStart(Session* pSession, int ID, char direction, short x, short y)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_MOVE_START, ID, direction, x, y);
	packet << ID << direction << x << y;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize(), pSession);
}

void Proxy::SC_MoveStop(Session* pSession, int ID, char direction, short x, short y)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_MOVE_STOP, ID, direction, x, y);
	packet << ID << direction << x << y;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize(), pSession);
}

void Proxy::SC_Attack1(int ID, char direction, short x, short y)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_ATTACK1, ID, direction, x, y);
	packet << ID << direction << x << y;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize());
}

void Proxy::SC_Attack2(int ID, char direction, short x, short y)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_ATTACK2, ID, direction, x, y);
	packet << ID << direction << x << y;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize());
}

void Proxy::SC_Attack3(int ID, char direction, short x, short y)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_ATTACK3, ID, direction, x, y);
	packet << ID << direction << x << y;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize());
}

void Proxy::SC_Damage(int attackID, int damageID, char damageHp)
{
	SerializePacket packet;
	CreateHeader(&packet, df_SC_DAMAGE, attackID, damageID, damageHp);
	packet << attackID << damageID << damageHp;
	_net->EnqueueBroadcast(packet.GetReadPtr(), packet.GetDataSize());
}
