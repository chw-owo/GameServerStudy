#include "JumpPointSearch.h"
#include "Node.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
using namespace std;

void JumpPointSearch::FindPath()
{
	if (!_bFindPathOn) return;

	_pNodeMgr->_checkedList.clear();
	_pNodeMgr->_checkedDiagList.clear();

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		if(_debugCreateNode) 
			printf("\nCur Node (%d, %d)\n", _pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);

		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("\nCan't Find Path!\n\n");
			_bFindPathOn = false;
			return;
		}
		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		if (_debugOpenList) PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();

	}
	else
	{
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
		CorrectPath();
		printf("\nComplete Find Path! (JPS: %d)\n\n", _pNodeMgr->_pDest->_g);
		_bFindPathOn = false;
	}
}

void JumpPointSearch::FindPathStepInto()
{
	if (!_bFindPathStepOn)
	{
		_pNodeMgr->SetData();
		_bFindPathStepOn = true;
		_bFindPathOn = false;
	}

	_pNodeMgr->_checkedList.clear();
	_pNodeMgr->_checkedDiagList.clear();

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		if (_debugCreateNode) 
			printf("\nCur Node (%d, %d)\n", _pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		
		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("\nCan't Find Path!\n\n");
			_bFindPathStepOn = false;
			return;
		}

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		if(_debugOpenList) PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();

	}
	else
	{
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
		CorrectPath();
		printf("\nComplete Find Path! (JPS: %d)\n\n", _pNodeMgr->_pDest->_g);
		_bFindPathStepOn = false;
	}
}

void JumpPointSearch::PrintDirForDebug(Node* pCurNode)
{
	switch (pCurNode->_dir)
	{
	case DIR::UP:
		printf("(%d, %d) is UP", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::R:
		printf("(%d, %d) is R", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::DOWN:
		printf("(%d, %d) is DOWN", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::L:
		printf("(%d, %d) is L", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::UP_R:
		printf("(%d, %d) is UP_R", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::DOWN_R:
		printf("(%d, %d) is DOWN_R", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::DOWN_L:
		printf("(%d, %d) is DOWN_L", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::UP_L:
		printf("(%d, %d) is UP_L", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	case DIR::NONE:
		printf("(%d, %d) is NONE", pCurNode->_pos._x, pCurNode->_pos._y);
		break;
	};

	switch (pCurNode->_searchDir)
	{
	case DIR::UP:
		printf(", UP");
		break;
	case DIR::R:
		printf(", R");
		break;
	case DIR::DOWN:
		printf(", DOWN");
		break;
	case DIR::L:
		printf(", L");
		break;
	case DIR::UP_R:
		printf(", UP_R");
		break;
	case DIR::DOWN_R:
		printf(", DOWN_R");
		break;
	case DIR::DOWN_L:
		printf(", DOWN_L");
		break;
	case DIR::UP_L:
		printf(", UP_L");
		break;
	case DIR::NONE:
		printf(", NONE");
		break;
	};

	printf("\n\n");
}

void JumpPointSearch::CheckCreateNode(Node* pCurNode)
{
	int newG = 0;
	Pos newPos = Pos(-1, 0);
	DIR dir = pCurNode->_dir;
	DIR searchDir = pCurNode->_searchDir;
	DIR newSearchDir = DIR::NONE; 

	if(_debugFindCorner) 
	{
		printf("\n");
		PrintDirForDebug(pCurNode);
		printf("\n");
	}

	switch (dir)
	{
	case DIR::UP:

		switch (searchDir)
		{
		case DIR::UP:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:
			if(CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if(CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);	
			}
			break;

		}
		break;

	case DIR::R:

		switch (searchDir)
		{
		case DIR::R:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;
		}
		break;

	case DIR::DOWN:

		switch (searchDir)
		{
		case DIR::DOWN:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}
			break;
		}
		break;

	case DIR::L:

		switch (searchDir)
		{
		case DIR::L:
			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;
		}
		break;

	case DIR::UP_R:

		switch (searchDir)
		{
		case DIR::UP:

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}
			break;

		case DIR::R:

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;

		case DIR::NONE:

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;

		case DIR::ALL:

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;
		}
		break;

	case DIR::DOWN_R:
		
		switch (searchDir)
		{
		case DIR::DOWN:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}
			break;

		case DIR::R:
			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;
		
		case DIR::NONE:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}
			break;

		case DIR::ALL:

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}
			break;
		}

		break;

	case DIR::DOWN_L:

		switch (searchDir)
		{
		case DIR::L:
			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;

		case DIR::NONE:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::ALL:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;
		}

		break;

	case DIR::UP_L:

		switch (searchDir)
		{
		case DIR::UP:
			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}
			break;

		case DIR::L:
			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:
			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;


		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;


		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::NONE:

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			break;

		case DIR::ALL:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;
		}

		break;

	case DIR::NONE:

		for (int i = (int)DIR::UP; i <= (int)DIR::L; i++)
		{
			if(CheckCorner(pCurNode->_pos, (DIR)i, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, true);
				CreateNode(pCurNode, newPos, (DIR)i, newSearchDir, newG);
			}
		}

		for (int i = (int)DIR::UP_R; i <= (int)DIR::UP_L; i++)
		{
			if (CheckCorner(pCurNode->_pos, (DIR)i, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos, false);
				CreateNode(pCurNode, newPos, (DIR)i, newSearchDir, newG);
			}
		}

		break;
	}	

	if (_debugFindCorner)
	{
		printf("\n");
	}
}

bool JumpPointSearch::CheckCorner(Pos curPos, DIR dir, Pos& newPos, DIR& searchDir)
{
	searchDir = DIR::NONE;

	switch (dir)
	{
	case DIR::UP:
	{
		Pos up = curPos + _direction[(int)dir];
		Pos up_r = up + _direction[(int)DIR::R];
		Pos up_l = up + _direction[(int)DIR::L];
		Pos up_r_front = up_r + _direction[(int)dir];
		Pos up_l_front = up_l + _direction[(int)dir];

		if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
		{
			if(_debugFindCorner) printf("UP: Obstacle (%d, %d)\n", up._x, up._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
		{
			newPos = up;
			searchDir = DIR::NONE;
			if(_debugFindCorner) printf("★★★ UP: Dest (%d, %d)\n", up._x, up._y);
			return true;
		}

		if (up._y < 0)
		{
			if(_debugFindCorner) printf("UP: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(up._x, up._y));

		while (up_r_front._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(up_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_r_front) != Map::OBSTACLE)
			{
				newPos = up;
				searchDir = DIR::UP_R;
			}

			if (_pNodeMgr->_pMap->GetMapState(up_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP_R)
				{
					searchDir = DIR::UP;
				}
				else
				{
					newPos = up;
					searchDir = DIR::UP_L;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (_debugFindCorner)
				{
					if (searchDir == DIR::UP)
						printf("★★★ UP: R L Corner (%d, %d)\n", up._x, up._y);
					else if (searchDir == DIR::UP_R)
						printf("★★★ UP: L Corner (%d, %d)\n", up._x, up._y);
					else if (searchDir == DIR::UP_L)
						printf("★★★ UP: R Corner (%d, %d)\n", up._x, up._y);
				}
				return true;
			}

			up = up + _direction[(int)dir];
			up_r = up + _direction[(int)DIR::R];
			up_l = up + _direction[(int)DIR::L];
			up_r_front = up_r + _direction[(int)dir];
			up_l_front = up_l + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("UP: Obstacle (%d, %d)\n", up._x, up._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
			{
				newPos = up;
				searchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ UP: Dest (%d, %d)\n", up._x, up._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(up._x, up._y));
		}

		if(_debugFindCorner) printf("UP: Range Out\n");
		return false;
	}
	break;

	case DIR::DOWN:
	{
		Pos down = curPos + _direction[(int)dir];
		Pos down_r = down + _direction[(int)DIR::R];
		Pos down_l = down + _direction[(int)DIR::L];
		Pos down_r_front = down_r + _direction[(int)dir];
		Pos down_l_front = down_l + _direction[(int)dir];

		if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
		{
			if(_debugFindCorner) printf("DOWN: Obstacle (%d, %d)\n", down._x, down._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
		{
			newPos = down;
			searchDir = DIR::NONE;
			if(_debugFindCorner) printf("★★★ DOWN: Dest (%d, %d)\n", down._x, down._y);
			return true;
		}

		if (down._y >= Y_MAX)
		{
			if(_debugFindCorner) printf("DOWN: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(down._x, down._y));

		while (down_r_front._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(down_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_r_front) != Map::OBSTACLE)
			{
				newPos = down;
				searchDir = DIR::DOWN_R;
			}

			if (_pNodeMgr->_pMap->GetMapState(down_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::DOWN_R)
				{
					searchDir = DIR::DOWN;
				}
				else
				{
					newPos = down;
					searchDir = DIR::DOWN_L;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (_debugFindCorner)
				{
					if (searchDir == DIR::DOWN)
						printf("★★★ DOWN: R L Corner (%d, %d)\n", down._x, down._y);
					else if (searchDir == DIR::DOWN_L)
						printf("★★★ DOWN: R Corner (%d, %d)\n", down._x, down._y);
					else if (searchDir == DIR::DOWN_R)
						printf("★★★ DOWN: L Corner (%d, %d)\n", down._x, down._y);
				}
				return true;
			}

			down = down + _direction[(int)dir];
			down_r = down + _direction[(int)DIR::R];
			down_l = down + _direction[(int)DIR::L];
			down_r_front = down_r + _direction[(int)dir];
			down_l_front = down_l + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("DOWN: Obstacle (%d, %d)\n", down._x, down._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
			{
				newPos = down;
				searchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ DOWN: Dest (%d, %d)\n", down._x, down._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(down._x, down._y));
		}

		if(_debugFindCorner) printf("DOWN: Range Out\n");
		return false;
	}
	break;

	case DIR::R:
	{
		Pos r = curPos + _direction[(int)dir];
		Pos r_up = r + _direction[(int)DIR::UP];
		Pos r_down = r + _direction[(int)DIR::DOWN];
		Pos r_up_front = r_up + _direction[(int)dir];
		Pos r_down_front = r_down + _direction[(int)dir];

		if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
		{
			if(_debugFindCorner) printf("R: Obstacle (%d, %d)\n", r._x, r._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
		{
			newPos = r;
			searchDir = DIR::NONE;
			if(_debugFindCorner) printf("★★★ R: Dest (%d, %d)\n", r._x, r._y);
			return true;
		}

		if (r._x >= X_MAX)
		{
			if(_debugFindCorner) printf("R: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(r._x, r._y));

		while (r_up_front._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(r_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_up_front) != Map::OBSTACLE)
			{
				newPos = r;
				searchDir = DIR::UP_R;
			}

			if (_pNodeMgr->_pMap->GetMapState(r_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP_R)
				{
					searchDir = DIR::R;
				}
				else
				{
					newPos = r;
					searchDir = DIR::DOWN_R;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (_debugFindCorner)
				{
					if (searchDir == DIR::R)
						printf("★★★ R: Up Down Corner (%d, %d)\n", r._x, r._y);
					else if (searchDir == DIR::DOWN_R)
						printf("★★★ R: UP Corner (%d, %d)\n", r._x, r._y);
					else if (searchDir == DIR::UP_R)
						printf("★★★ R: Down Corner (%d, %d)\n", r._x, r._y);
				}
				return true;
			}

			r = r + _direction[(int)dir];
			r_up = r + _direction[(int)DIR::UP];
			r_down = r + _direction[(int)DIR::DOWN];
			r_up_front = r_up + _direction[(int)dir];
			r_down_front = r_down + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("R: Obstacle (%d, %d)\n", r._x, r._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
			{
				newPos = r;
				searchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ R: Dest (%d, %d)\n", r._x, r._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(r._x, r._y));
		}

		if(_debugFindCorner) printf("R: Range Out\n");
		return false;
	}
	break;

	case DIR::L:
	{
		Pos l = curPos + _direction[(int)dir];
		Pos l_up = l + _direction[(int)DIR::UP];
		Pos l_down = l + _direction[(int)DIR::DOWN];
		Pos l_up_front = l_up + _direction[(int)dir];
		Pos l_down_front = l_down + _direction[(int)dir];

		if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
		{
			if(_debugFindCorner) printf("L: Obstacle (%d, %d)\n", l._x, l._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
		{
			newPos = l;
			searchDir = DIR::NONE;
			if(_debugFindCorner) printf("★★★ L: Dest (%d, %d)\n", l._x, l._y);
			return true;
		}

		if (l._x < 0)
		{
			if(_debugFindCorner) printf("L: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(l._x, l._y));

		while (l_up_front._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(l_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_up_front) != Map::OBSTACLE)
			{
				newPos = l;
				searchDir = DIR::UP_L;
			}

			if (_pNodeMgr->_pMap->GetMapState(l_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP_L)
				{
					searchDir = DIR::L;
				}
				else
				{
					newPos = l;
					searchDir = DIR::DOWN_L;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (_debugFindCorner)
				{
					if (searchDir == DIR::L)
						printf("★★★ L: Up Down Corner (%d, %d)\n", l._x, l._y);
					else if (searchDir == DIR::DOWN_L)
						printf("★★★ L: Up Corner (%d, %d)\n", l._x, l._y);
					else if (searchDir == DIR::UP_L)
						printf("★★★ L: Down Corner (%d, %d)\n", l._x, l._y);
				}
				return true;
			}

			l = l + _direction[(int)dir];
			l_up = l + _direction[(int)DIR::UP];
			l_down = l + _direction[(int)DIR::DOWN];
			l_up_front = l_up + _direction[(int)dir];
			l_down_front = l_down + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("L: Obstacle (%d, %d)\n", l._x, l._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
			{
				newPos = l;
				searchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ L: Dest (%d, %d)\n", l._x, l._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(l._x, l._y));
		}

		if(_debugFindCorner) printf("L: Range Out\n");
		return false;
	}
	break;

	case DIR::UP_R:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_down = diag + _direction[(int)DIR::DOWN];

		while (diag._x < X_MAX && diag._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("UP_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::UP_R;
				if(_debugFindCorner) printf("★★★ UP_R: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::ALL;
				if(_debugFindCorner) printf("★★★ UP_R: UP_R (%d, %d)\n", diag._x, diag._y);
				return true;
			}
			
			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::UP_R, DIR::R, newPos, searchDir))
			{
				if (searchDir == DIR::UP_R)
				{
					searchDir = DIR::DOWN_R;
				}
				return true;
			}

			if (CheckDiagCorner(diag, DIR::UP_R, DIR::UP, newPos, searchDir))
			{
				if (searchDir == DIR::UP_R)
				{
					searchDir = DIR::UP_L;
				}
				return true;
			}

			diag = diag + _direction[(int)dir];
			diag_l = diag + _direction[(int)DIR::L];
			diag_down = diag + _direction[(int)DIR::DOWN];
		}

		if(_debugFindCorner) printf("UP_R: Range Out\n");
		return false;
	}
	break;

	case DIR::DOWN_R:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_up = diag + _direction[(int)DIR::UP];
		
		while (diag._x < X_MAX && diag._y < Y_MAX)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("DOWN_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::DOWN_R;
				if(_debugFindCorner) printf("★★★ DOWN_R: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::ALL;
				if(_debugFindCorner) printf("★★★ DOWN_R: DOWN_R (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::DOWN_R, DIR::R, newPos, searchDir))
			{
				if (searchDir == DIR::DOWN_R)
				{
					searchDir = DIR::UP_R;
				}
				return true;
			}

			if (CheckDiagCorner(diag, DIR::DOWN_R, DIR::DOWN, newPos, searchDir))
			{
				if (searchDir == DIR::DOWN_R)
				{
					searchDir = DIR::DOWN_L;
				}
				return true;
			}

			diag = diag + _direction[(int)dir];
			diag_l = diag + _direction[(int)DIR::L];
			diag_up = diag + _direction[(int)DIR::UP];

		}
		if(_debugFindCorner) printf("DOWN_R: Range Out\n");
		return false;
	}
	break;

	case DIR::DOWN_L:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_up = diag + _direction[(int)DIR::UP];

		while (diag._x >= 0 && diag._y < Y_MAX)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("DOWN_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::DOWN_L;
				if(_debugFindCorner) printf("★★★ DOWN_L: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::ALL;
				if(_debugFindCorner) printf("★★★ DOWN_L: DOWN_L (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::DOWN_L, DIR::L, newPos, searchDir))
			{
				if (searchDir == DIR::DOWN_L)
				{
					searchDir = DIR::UP_L;
				}
				return true;
			}

			if (CheckDiagCorner(diag, DIR::DOWN_L, DIR::DOWN, newPos, searchDir))
			{
				if (searchDir == DIR::DOWN_L)
				{
					searchDir = DIR::DOWN_R;
				}
				return true;
			}

			diag = diag + _direction[(int)dir];
			diag_r = diag + _direction[(int)DIR::R];
			diag_up = diag + _direction[(int)DIR::UP];
		}
		if(_debugFindCorner) printf("DOWN_L: Range Out\n");
		return false;
	}
	break;

	case DIR::UP_L:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_down = diag + _direction[(int)DIR::DOWN];

		while (diag._x >= 0 && diag._y >= 0)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				if(_debugFindCorner) printf("UP_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::UP_L;
				if(_debugFindCorner) printf("★★★ UP_L: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::ALL;
				if(_debugFindCorner) printf("★★★ UP_L: UP_L (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::UP_L, DIR::L, newPos, searchDir))
			{
				if (searchDir == DIR::UP_L)
				{
					searchDir = DIR::DOWN_L;
				}
				return true;
			}

			if (CheckDiagCorner(diag, DIR::UP_L, DIR::UP, newPos, searchDir)) 
			{
				if (searchDir == DIR::UP_L)
				{
					searchDir = DIR::UP_R;
				}
				return true;
			}

			diag = diag + _direction[(int)dir];
			diag_r = diag + _direction[(int)DIR::R];
			diag_down = diag + _direction[(int)DIR::DOWN];
		}
		if(_debugFindCorner) printf("UP_L: Range Out\n");
		return false;
	}
	break;
	}
}

bool JumpPointSearch::CheckDiagCorner(Pos diag, DIR diagDir, DIR searchDir, Pos& newPos, DIR& newSearchDir)
{
	DIR includeDir;
	DIR excludeDir;

	char dirText[8] = {'\0',};

	if (_debugFindCorner)
	{
		switch (diagDir)
		{
		case DIR::UP_R:
			strcpy_s(dirText, 8, "UP_R");
			break;

		case DIR::DOWN_R:
			strcpy_s(dirText, 8, "DOWN_R");
			break;

		case DIR::DOWN_L:
			strcpy_s(dirText, 8, "DOWN_L");
			break;

		case DIR::UP_L:
			strcpy_s(dirText, 8, "UP_L");
			break;
		}
	}

	switch (searchDir)
	{
	case DIR::R:
	{
		if(diagDir == DIR::UP_R)
		{
			includeDir = DIR::DOWN;
			excludeDir = DIR::UP;
		}
		else if (diagDir == DIR::DOWN_R)
		{
			includeDir = DIR::UP;
			excludeDir = DIR::DOWN;
		}

		Pos r = diag;
		Pos r_includeDir = r + _direction[(int)includeDir];
		Pos r_includeDir_front = r_includeDir + _direction[(int)searchDir];

		if (r_includeDir_front._x < X_MAX &&
			_pNodeMgr->_pMap->GetMapState(r_includeDir) == Map::OBSTACLE &&
			_pNodeMgr->_pMap->GetMapState(r_includeDir_front) != Map::OBSTACLE)
		{
			newPos = diag;
			newSearchDir = diagDir;
			if(_debugFindCorner) printf("★★★ %s: R, Side (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, r_includeDir._x, r_includeDir._y);
			if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(r._x, r._y, newPos));
			return true;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(r._x, r._y));

		r = r + _direction[(int)searchDir];
		r_includeDir = r + _direction[(int)includeDir];
		r_includeDir_front = r_includeDir + _direction[(int)searchDir];
		Pos r_excludeDir = r + _direction[(int)excludeDir];
		Pos r_excludeDir_front = r_excludeDir + _direction[(int)searchDir];

		if (r._x >= X_MAX)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::R;
			if(_debugFindCorner) printf("★★★ %s: R, Dest (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, r._x, r._y);
			return true;
		}

		while (r_includeDir_front._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(r_includeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_includeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: R, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, r_includeDir._x, r_includeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(r._x, r._y, newPos));
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(r_excludeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_excludeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: R, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, r_excludeDir._x, r_excludeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(r._x, r._y, newPos));
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(r._x, r._y));

			r = r + _direction[(int)searchDir];
			r_includeDir = r + _direction[(int)includeDir];
			r_includeDir_front = r_includeDir + _direction[(int)searchDir];
			r_excludeDir = r + _direction[(int)excludeDir];
			r_excludeDir_front = r_excludeDir + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::R;
				if(_debugFindCorner) printf("★★★ %s: R, Dest (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, r._x, r._y);
				return true;
			}
		}
		
		_pNodeMgr->_checkedDiagList.push_back(Pos(r._x, r._y));

		return false;
	}
	break;

	case DIR::UP:
	{
		if (diagDir == DIR::UP_R)
		{
			includeDir = DIR::L;
			excludeDir = DIR::R;
		}
		else if (diagDir == DIR::UP_L)
		{
			includeDir = DIR::R;
			excludeDir = DIR::L;
		}

		Pos up = diag;
		Pos up_includeDir = up + _direction[(int)includeDir];
		Pos up_includeDir_front = up_includeDir + _direction[(int)searchDir];

		if (up_includeDir_front._y >= 0 &&
			_pNodeMgr->_pMap->GetMapState(up_includeDir) == Map::OBSTACLE &&
			_pNodeMgr->_pMap->GetMapState(up_includeDir_front) != Map::OBSTACLE)
		{
			newPos = diag;
			newSearchDir = diagDir;
			if(_debugFindCorner) printf("★★★ %s: UP, Side (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, up_includeDir._x, up_includeDir._y);
			if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(up._x, up._y, newPos));
			return true;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(up._x, up._y));

		up = up + _direction[(int)searchDir];
		up_includeDir = up + _direction[(int)includeDir];
		up_includeDir_front = up_includeDir + _direction[(int)searchDir];
		Pos up_excludeDir = up + _direction[(int)excludeDir];
		Pos up_excludeDir_front = up_excludeDir + _direction[(int)searchDir];

		if (up._y < 0)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::UP;
			if(_debugFindCorner) printf("★★★ %s: UP, Dest (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, up._x, up._y);
			return true;
		}

		while (up_includeDir_front._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(up_includeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_includeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: UP, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, up_includeDir._x, up_includeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(up._x, up._y, newPos));
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(up_excludeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_excludeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: UP, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, up_excludeDir._x, up_excludeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(up._x, up._y, newPos));
				return true;
			}
		
			_pNodeMgr->_checkedDiagList.push_back(Pos(up._x, up._y));

			up = up + _direction[(int)searchDir];
			up_includeDir = up + _direction[(int)includeDir];
			up_includeDir_front = up_includeDir + _direction[(int)searchDir];
			up_excludeDir = up + _direction[(int)excludeDir];
			up_excludeDir_front = up_excludeDir + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::UP;
				if(_debugFindCorner) printf("★★★ %s: UP, Dest (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, up._x, up._y);
				return true;
			}
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(up._x, up._y));
		return false;
	}
		break;

	case DIR::L:
	{
		if (diagDir == DIR::UP_L)
		{
			includeDir = DIR::DOWN;
			excludeDir = DIR::UP;
		}
		else if (diagDir == DIR::DOWN_L)
		{
			includeDir = DIR::UP;
			excludeDir = DIR::DOWN;
		}

		Pos l = diag;
		Pos l_includeDir = l + _direction[(int)includeDir];
		Pos l_includeDir_front = l_includeDir + _direction[(int)searchDir];

		if (l_includeDir_front._x >= 0 &&
			_pNodeMgr->_pMap->GetMapState(l_includeDir) == Map::OBSTACLE &&
			_pNodeMgr->_pMap->GetMapState(l_includeDir_front) != Map::OBSTACLE)
		{
			newPos = diag;
			newSearchDir = diagDir;
			if(_debugFindCorner) printf("★★★ %s: L, Side (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, l_includeDir._x, l_includeDir._y);
			if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(l._x, l._y, newPos));
			return true;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(l._x, l._y));

		l = l + _direction[(int)searchDir];
		l_includeDir = l + _direction[(int)includeDir];
		l_includeDir_front = l_includeDir + _direction[(int)searchDir];
		Pos l_excludeDir = l + _direction[(int)excludeDir];
		Pos l_excludeDir_front = l_excludeDir + _direction[(int)searchDir];
		
		if (l._x < 0)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::L;
			if(_debugFindCorner) printf("★★★ %s: L, Dest (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, l._x, l._y);
			return true;
		}

		while (l_includeDir_front._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(l_includeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_includeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: L, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, l_includeDir._x, l_includeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(l._x, l._y, newPos));
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(l_excludeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_excludeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: L, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, l_excludeDir._x, l_excludeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(l._x, l._y, newPos));
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(l._x, l._y));

			l = l + _direction[(int)searchDir];
			l_includeDir = l + _direction[(int)includeDir];
			l_includeDir_front = l_includeDir + _direction[(int)searchDir];
			l_excludeDir = l + _direction[(int)excludeDir];
			l_excludeDir_front = l_excludeDir + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::L;
				if(_debugFindCorner) printf("★★★ %s: L, Dest (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, l._x, l._y);
				return true;
			}
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(l._x, l._y));
		return false;
	}
		break;
	
	case DIR::DOWN:
	{
		if (diagDir == DIR::DOWN_R)
		{
			includeDir = DIR::L;
			excludeDir = DIR::R;
		}
		else if (diagDir == DIR::DOWN_L)
		{
			includeDir = DIR::R;
			excludeDir = DIR::L;
		}

		Pos down = diag;
		Pos down_includeDir = down + _direction[(int)includeDir];
		Pos down_includeDir_front = down_includeDir + _direction[(int)searchDir];

		if (down_includeDir_front._y < Y_MAX &&
			_pNodeMgr->_pMap->GetMapState(down_includeDir) == Map::OBSTACLE &&
			_pNodeMgr->_pMap->GetMapState(down_includeDir_front) != Map::OBSTACLE)
		{
			newPos = diag;
			newSearchDir = diagDir;
			if(_debugFindCorner) printf("★★★ %s: Down, Side (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, down_includeDir._x, down_includeDir._y);
			if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(down._x, down._y, newPos));
			return true;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(down._x, down._y));

		down = down + _direction[(int)searchDir];
		down_includeDir = down + _direction[(int)includeDir];
		down_includeDir_front = down_includeDir + _direction[(int)searchDir];
		Pos down_excludeDir = down + _direction[(int)excludeDir];
		Pos down_excludeDir_front = down_excludeDir + _direction[(int)searchDir];

		if (down._y >= Y_MAX)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::DOWN;
			if(_debugFindCorner) printf("★★★ %s: Down, Dest (%d, %d) (%d, %d)\n", dirText,
				diag._x, diag._y, down._x, down._y);
			return true;
		}

		while (down_includeDir_front._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(down_includeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_includeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: Down, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, down_includeDir._x, down_includeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(down._x, down._y, newPos));
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(down_excludeDir) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_excludeDir_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::NONE;
				if(_debugFindCorner) printf("★★★ %s: Down, Side (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, down_excludeDir._x, down_excludeDir._y);
				if (_debugFindCorner) _pNodeMgr->_diagCuzList.push_back(DiagCuz(down._x, down._y, newPos));
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(down._x, down._y));

			down = down + _direction[(int)searchDir];
			down_includeDir = down + _direction[(int)includeDir];
			down_includeDir_front = down_includeDir + _direction[(int)searchDir];
			down_excludeDir = down + _direction[(int)excludeDir];
			down_excludeDir_front = down_excludeDir + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::DOWN;
				if(_debugFindCorner) printf("★★★ %s: Down, Dest (%d, %d) (%d, %d)\n", dirText,
					diag._x, diag._y, down._x, down._y);
				return true;
			}
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(down._x, down._y));
		return false;
	}
		break;
	}
}

void JumpPointSearch::CreateNode(Node* pCurNode, Pos newPos, DIR dir, DIR searchDir, int newG)
{
	Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);

	switch (state)
	{
	case Map::NONE:
	case Map::START:
	case Map::DEST:
	{
		Node* pNew = new Node(
			newPos,
			newG,
			newPos.GetDistanceToDest(_pNodeMgr->_pMap->_destPos),
			dir,
			searchDir,
			pCurNode
		);

		if (_debugCreateNode) 
		{
			printf("Create New Node (%d, %d) (parent: %d, %d) (%d + %d = %d)\n",
				pNew->_pos._x, pNew->_pos._y,
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y,
				pNew->_g, pNew->_h, pNew->_f);
		}

		_pNodeMgr->_openList.push_back(pNew);
		_pNodeMgr->_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
	}
	break;

	case Map::OPEN:
	{
		compareG.pos = newPos;
		compareG.g = newG;

		vector<Node*>::iterator it = find_if(_pNodeMgr->_openList.begin(),
			_pNodeMgr->_openList.end(), compareG);

		if (it != _pNodeMgr->_openList.end())
		{
			(*it)->ResetParent(newG, dir, searchDir, pCurNode);
			if (_debugCreateNode) printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
				newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			if (_debugCreateNode) printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
		}
	}
	break;

	case Map::CLOSE:
	{
		compareG.pos = newPos;
		compareG.g = newG;

		vector<Node*>::iterator it = find_if(_pNodeMgr->_closeList.begin(),
			_pNodeMgr->_closeList.end(), compareG);

		if (it != _pNodeMgr->_closeList.end())
		{
			(*it)->ResetParent(newG, dir, searchDir, pCurNode);
			if (_debugCreateNode) printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
				newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			if (_debugCreateNode) printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
		}
	}
	break;

	case Map::OBSTACLE:
	case Map::RANGE_OUT:
		if (_debugCreateNode) printf("Can't Go (%d, %d)\n", newPos._x, newPos._y);
		break;

	}
}

void JumpPointSearch::CorrectPath()
{
	Node* pNode1 = _pNodeMgr->_pDest;
	Node* pNode2 = pNode1->_pParent;
	Node* pPrevNode = nullptr;

	_pNodeMgr->_correctedList.push_back(pNode1);

	while (pNode2 != _pNodeMgr->_pStart)
	{
		if (CheckObstacle(pNode1, pNode2))
		{
			printf("(%d, %d) - (%d, %d): Obstacle!! ",
				pNode1->_pos._x, pNode1->_pos._y, pNode2->_pos._x, pNode2->_pos._y);

			if (pPrevNode == nullptr) return;

			_pNodeMgr->_correctedList.push_back(pPrevNode);
			pNode1 = pPrevNode;
			pNode2 = pPrevNode->_pParent;

			printf("Change Node (%d, %d) - (%d, %d)\n",
				pNode1->_pos._x, pNode1->_pos._y, pNode2->_pos._x, pNode2->_pos._y);

		}
		else
		{
			printf("(%d, %d) - (%d, %d): Can pass. ",
				pNode1->_pos._x, pNode1->_pos._y, pNode2->_pos._x, pNode2->_pos._y);

			pPrevNode = pNode2;
			pNode2 = pNode2->_pParent;

			printf("Change Node (%d, %d) - (%d, %d)\n",
				pNode1->_pos._x, pNode1->_pos._y, pNode2->_pos._x, pNode2->_pos._y);
		}
	}

	_pNodeMgr->_correctedList.push_back(pNode2);
}

bool JumpPointSearch::CheckObstacle(Node* pNode1, Node* pNode2)
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
			curY += yUnit;
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
		}
		return false;
	}

	if (dY == 0)
	{
		int xUnit = dX / abs(dX);
		while (curX != destX)
		{
			curX += xUnit;
			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
		}
		return false;
	}

	int accX = 0;
	int accY = 0;
	int	xUnit = dX / abs(dX);
	int yUnit = dY / abs(dY);

	if (abs(dX) >= abs(dY))
	{
		int dX1 = abs(dX / 2);
		int dY1 = abs(dY / 2);

		while (accY < dY1)
		{
			accX += dY1;
			curX += xUnit;

			if (accX > dX1)
			{
				accX -= dX1;
				accY++;
				curY += yUnit;
			}

			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
		}

		curX += xUnit;
		curY += yUnit;

		if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
		{
			return true;
		}

		accX = 0;
		accY = dY1 + 1;
		int dX2 = abs(dX) - dX1 + 1;
		int dY2 = abs(dY) - dY1 + 1;

		while (accY < dY2)
		{
			accX += dY2;
			curX += xUnit;

			if (accX > dX2)
			{
				accX -= dX2;
				accY++;
				curY += yUnit;
			}

			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		int dX1 = abs(dX / 2);
		int dY1 = abs(dY / 2);

		while (accX < dX1)
		{
			accY += dX1;
			curY += yUnit;

			if (accY > dY1)
			{
				accY -= dY1;
				accX++;
				curX += xUnit;
			}

			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}
		}

		curX += xUnit;
		curY += yUnit;

		if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
		{
			return true;
		}

		accX = dX1 + 1;
		accY = 0;
		int dX2 = abs(dX) - dX1 + 1;
		int dY2 = abs(dY) - dY1 + 1;

		while (accX < dX2)
		{
			accY += dX2;
			curY += yUnit;

			if (accY > dY2)
			{
				accY -= dY2;
				accX++;
				curX += xUnit;
			}

			if (_pNodeMgr->_pMap->GetMapState(curX, curY) == Map::OBSTACLE)
			{
				return true;
			}

		}

		return false;
	}

	return false;
}

void JumpPointSearch::PrintOpenListForDebug()
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


bool JumpPointSearch::CompareG::operator()(Node*& pNode) const
{
	if (pNode->_pos == pos)
	{
		printf("(%d, %d) %d, %d\n", pNode->_pos._x, pNode->_pos._y, pNode->_g, g);
	}

	return (pNode->_pos == pos && pNode->_g >= g);
}