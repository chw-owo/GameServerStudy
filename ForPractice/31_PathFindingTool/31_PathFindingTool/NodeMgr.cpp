#include "NodeMgr.h"
#include <algorithm>

bool CompareF::operator() (Node* const left, Node* const right) const
{
	return left->_f > right->_f;
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

void NodeMgr::SetData()
{
	_pMap->ClearOpenCloseState();

	while (!_openList.empty())
	{
		Node* pNode = _openList.back();
		delete pNode;
		_openList.pop_back();
	}

	while (!_closeList.empty())
	{
		Node* pNode = _closeList.back();
		delete pNode;
		_closeList.pop_back();
	}

	_pDest = nullptr;
	_pStart = new Node(_pMap->_startPos, 0, _pMap->_startPos.GetDistance(_pMap->_destPos));
	printf("\ncreate start node (%d, %d)\n", _pStart->_pos._x, _pStart->_pos._y);
}

