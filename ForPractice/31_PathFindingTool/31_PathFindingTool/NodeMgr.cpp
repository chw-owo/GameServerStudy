#include "NodeMgr.h"

bool cmp::operator() (Node* const left, Node* const right) const
{
	if (left->_f != right->_f)
	{
		return left->_f > right->_f;
	}
	else
	{
		return (__int64)left > (__int64)right;
	}
}

NodeMgr::NodeMgr()
{
	_pMap = Map::GetInstance();
}

NodeMgr* NodeMgr::GetInstance()
{
	static NodeMgr _NodeMgr;
	return &_NodeMgr;
}

void NodeMgr::SetStartDestNode()
{
	_pDest = nullptr;

	if (_pStart == nullptr)
	{
		printf("create start node (%d, %d)\n", _pMap->_startPos._x, _pMap->_startPos._y);
		_pStart = new Node(_pMap->_startPos, 0, _pMap->_startPos.GetDistance(_pMap->_destPos));
	}
	else
	{
		printf("reset start node (%d, %d)\n", _pMap->_startPos._x, _pMap->_startPos._y);
		_pStart->SetData(_pMap->_startPos, 0, _pMap->_startPos.GetDistance(_pMap->_destPos));
	}
	
	_openSet.push(_pStart);
}

