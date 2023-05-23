#include "GraphicObject.h"

CGraphicObject::CGraphicObject(int x, int y, char icon): _chIcon(icon)
{
	_ObjectType = GRAPHIC;
	_iX = x; 
	_iY = y;

	_pScreenBuffer = CScreenBuffer::GetInstance();
}

CGraphicObject::CGraphicObject()
{

}

CGraphicObject::~CGraphicObject()
{

}

void CGraphicObject::Update()
{

}
void CGraphicObject::Render()
{
	_pScreenBuffer->SpriteDraw(_iX, _iY, _chIcon);
}

void CGraphicObject::OnCollision(CBaseObject* pTarget)
{

}