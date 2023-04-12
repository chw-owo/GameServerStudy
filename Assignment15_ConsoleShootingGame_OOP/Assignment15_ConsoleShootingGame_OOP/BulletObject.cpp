#include "BulletObject.h"

clock_t CBulletObject::_coolTimeCheck = clock();

CBulletObject::CBulletObject(char icon, int attack, int speed, CPlayerObject* player)
	:_chIcon(icon), _iAttack(attack), _fSpeed(1/(float)speed), _pPlayer(player)
{
	_ObjectType = BULLET;
	Deactivate();
	_pScreenBuffer = CScreenBuffer::GetInstance();
}

CBulletObject::CBulletObject()
{

}

CBulletObject::~CBulletObject()
{

}

int CBulletObject::GetAttack()
{
	return _iAttack;
}

void CBulletObject::Deactivate()
{
	_bDead = true;
	_iX = _pPlayer->GetX();
	_iY = _pPlayer->GetY();
}

void CBulletObject::Update()
{
	float time = (float)(clock() - _moveStart) / CLOCKS_PER_SEC;
	float coolTime = (float)(clock() - _coolTimeCheck) / CLOCKS_PER_SEC;

	if (time < _fSpeed)
		return;

	if (_bDead && coolTime >= _fSpeed)
	{
		_bDead = false;
		_iX = _pPlayer->GetX();
		_iY--;
		_coolTimeCheck = clock();
	}
	else if(!_bDead)
	{
		if (_iY >= 0)
			_iY--;
		else
			Deactivate();
	}
	
	_moveStart = clock();
}

void CBulletObject::Render()
{
	if(!_bDead)
		_pScreenBuffer->SpriteDraw(_iX, _iY, _chIcon);
}

void CBulletObject::OnCollision(CBaseObject* pTarget)
{
	if (pTarget->GetObjectType() == ENEMY)
	{
		Deactivate();
	}
}