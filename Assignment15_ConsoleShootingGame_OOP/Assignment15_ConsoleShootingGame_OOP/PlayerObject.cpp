#include "PlayerObject.h"
#include "ScreenBuffer.h"

CPlayerObject::CPlayerObject(char icon, int hp, int speed)
	: _chIcon(icon), _iTotalHp(hp), _iHp(hp), _fSpeed(1/(float)speed)
{
	_ObjectType = PLAYER;
	_iX = dfSCREEN_WIDTH / 2;
	_iY = (dfSCREEN_HEIGHT / 4) * 3;

	_pScreenBuffer = CScreenBuffer::GetInstance();
	_moveStart = clock();
}

CPlayerObject::CPlayerObject()
{

}

CPlayerObject::~CPlayerObject()
{

}

void CPlayerObject::GetKeyInput()
{
	float time = (float)(clock() - _moveStart) / CLOCKS_PER_SEC;

	if (GetAsyncKeyState(VK_LEFT))
	{
		if ( _iX > 0 && time > _fSpeed)
		{
			_iX--;
			_moveStart = clock();
		}
	}
	else if (GetAsyncKeyState(VK_RIGHT))
	{
		if (_iX < dfSCREEN_WIDTH - 3 && time >_fSpeed)
		{
			_iX++;
			_moveStart = clock();
		}
	}
}

void CPlayerObject::Update()
{
	GetKeyInput();
}

void CPlayerObject::Render()
{
	_pScreenBuffer->SpriteDraw(_iX, _iY, _chIcon);
}

void CPlayerObject::OnCollision(CBaseObject* pTarget)
{

}