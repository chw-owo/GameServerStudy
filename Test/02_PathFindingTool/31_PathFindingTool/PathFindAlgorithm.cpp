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

void PathFindAlgorithm::CorrectPath()
{
	Node* pNode1 = _pNodeMgr->_pDest;
	Node* pNode2 = pNode1->_pParent;
	Node* pPrevNode = pNode2;

	_pNodeMgr->_correctedList.push_back(pNode1);

	while (pNode2 != _pNodeMgr->_pStart)
	{
		if (CheckObstacle(pNode1, pNode2))
		{
			_pNodeMgr->_correctedList.push_back(pPrevNode);
			pNode1 = pPrevNode;
			pNode2 = pPrevNode->_pParent;
			pPrevNode = pNode2;
		}
		else
		{
			pPrevNode = pNode2;
			pNode2 = pNode2->_pParent;
		}
	}

	if (CheckObstacle(pNode1, pNode2))
	{
		_pNodeMgr->_correctedList.push_back(pPrevNode);
	}

	_pNodeMgr->_correctedList.push_back(pNode2);
}

bool PathFindAlgorithm::CheckObstacle(Node* pNode1, Node* pNode2)
{
	int curX = pNode1->_pos._x;
	int curY = pNode1->_pos._y;
	int destX = pNode2->_pos._x;
	int destY = pNode2->_pos._y;

	int dX = destX - curX;
	int dY = destY - curY;

	if (dX == 0)
	{
		int yUnit = dY / abs(dY);
		while (curY != destY)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
			curY += yUnit;
		}
		return false;
	}

	if (dY == 0)
	{
		int xUnit = dX / abs(dX);
		while (curX != destX)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

			curX += xUnit;
		}
		return false;
	}

	int accX = 0;
	int accY = 0;
	int	xUnit = dX / abs(dX);
	int yUnit = dY / abs(dY);

	dX = abs(dX) + 1;
	dY = abs(dY) + 1;

	if (dX >= dY)
	{
		int dYUnit = (dY + 1) / 2;

		int dX1 = dX / 2;
		int dY1 = (dY + 1) / 2;

		while (accY < dY1)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

			accX += dYUnit;
			curX += xUnit;

			if (accX >= dX1)
			{
				accX -= dX1;
				accY++;
				curY += yUnit;
			}
		}

		accX = 0;
		accY = 0;
		int dX2 = dX - dX1;
		int dY2 = dY - dY1;

		if (dY1 != dY2)
		{
			curY -= yUnit;
			accY--;
		}

		while (accY < dY2)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

			accX += dYUnit;
			curX += xUnit;

			if (accX >= dX2)
			{
				accX -= dX2;
				accY++;
				curY += yUnit;
			}
		}

		return false;
	}
	else
	{
		int dXUnit = (dX + 1) / 2;

		int dX1 = (dX + 1) / 2;
		int dY1 = dY / 2;

		while (accX < dX1)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

			accY += dXUnit;
			curY += yUnit;

			if (accY >= dY1)
			{
				accY -= dY1;
				accX++;
				curX += xUnit;
			}
		}

		accX = 0;
		accY = 0;
		int dX2 = dX - dX1;
		int dY2 = dY - dY1;

		if (dX1 != dX2)
		{
			curX -= xUnit;
			accX--;
		}

		while (accX < dX2)
		{
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

			accY += dXUnit;
			curY += yUnit;

			if (accY >= dY2)
			{
				accY -= dY2;
				accX++;
				curX += xUnit;
			}
		}

		return false;
	}

	return false;
}

void PathFindAlgorithm::PrintOpenListForDebug()
{
	printf("\n=====================================\n");
	printf("<Open List>\n\n");
	for (int i = 0; i < _pNodeMgr->_openList.size(); i++)
	{
		printf("(%d, %d) : %d + %d = %d\n",
			_pNodeMgr->_openList[i]->_pos._x,
			_pNodeMgr->_openList[i]->_pos._y,
			_pNodeMgr->_openList[i]->_g,
			_pNodeMgr->_openList[i]->_h,
			_pNodeMgr->_openList[i]->_f);
	}

	printf("\nCurNode : (%d, %d), %d\n",
		_pNodeMgr->_pCurNode->_pos._x,
		_pNodeMgr->_pCurNode->_pos._y,
		_pNodeMgr->_pCurNode->_f);

	printf("=====================================\n");
}