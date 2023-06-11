#include "PathFindAlgorithm.h"

PathFindAlgorithm::PathFindAlgorithm()
{
	_pMap = Map::GetInstance();
	_pNodeMgr = NodeMgr::GetInstance();
}

void PathFindAlgorithm::StartFindPath()
{
	_bOn = true;
	_pNodeMgr->SetStartDestNode(_pMap->_startPos, _pMap->_destPos);
}


