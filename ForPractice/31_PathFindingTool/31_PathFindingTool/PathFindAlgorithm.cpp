#include "PathFindAlgorithm.h"

PathFindAlgorithm::PathFindAlgorithm()
{
	_pNodeMgr = NodeMgr::GetInstance();
}

void PathFindAlgorithm::StartFindPath()
{
	_pNodeMgr->SetData();
	_bFindPathStepOn = false;
	_bFindPathOn = true;
}


