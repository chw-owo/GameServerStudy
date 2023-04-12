#include "BaseObject.h"

CBaseObject::CBaseObject()
{
}

CBaseObject::~CBaseObject()
{
}

int CBaseObject::GetX()
{
	return _iX;
}

int CBaseObject::GetY()
{
	return _iY;
}

bool CBaseObject::GetDead()
{
	return _bDead;
}
CBaseObject::ObjectType CBaseObject::GetObjectType(void)
{
	return _ObjectType;
}