#include "EnemyObject.h"
#include "BulletObject.h"

int CEnemyObject::_iCnt = 0;

CEnemyObject::CEnemyObject(int x, int y, char type, char icon, int hp)
	: _chType(type), _chIcon(icon), _iHp(hp)
{
	_ObjectType = ENEMY;
	_iX = x;
	_iY = y;
	_iCnt++;
	_pScreenBuffer = CScreenBuffer::GetInstance();
}

CEnemyObject::CEnemyObject()
{

}

CEnemyObject::~CEnemyObject()
{
	
}

void CEnemyObject::Update()
{
	if (!_bDead && _iHp <= 0)
	{
		_bDead = true;
		_iCnt--;
	}
}

void CEnemyObject::Render()
{
	if (!_bDead)
		_pScreenBuffer->SpriteDraw(_iX, _iY, _chIcon);
}

void CEnemyObject::OnCollision(CBaseObject* pTarget)
{
	if (pTarget->GetObjectType() == BULLET)
	{
		_iHp -= ((CBulletObject*)pTarget)->GetAttack();
	}
}