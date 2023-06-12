#include "PathFindAlgorithm.h"

PathFindAlgorithm::PathFindAlgorithm()
{
	_pNodeMgr = NodeMgr::GetInstance();
}

void PathFindAlgorithm::StartFindPath()
{
	_bOn = true;
	_pNodeMgr->SetStartDestNode();
}


