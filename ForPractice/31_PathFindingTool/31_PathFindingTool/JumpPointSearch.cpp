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
		printf("\n");
		//printf("Cur Node (%d, %d)\n", _pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);

		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("\nCan't Find Path!\n");
			_bFindPathOn = false;
			return;
		}

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		//PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();

		printf("\n");
	}
	else
	{
		printf("\nComplete Find Path\n=================================\n");
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
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
		printf("\n");
		//printf("Cur Node (%d, %d)\n", _pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		
		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("\nCan't Find Path!\n");
			_bFindPathStepOn = false;
			return;
		}

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		//PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();
	}
	else
	{
		printf("\nComplete Find Path\n=================================\n");
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
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

	PrintDirForDebug(pCurNode);

	switch (dir)
	{
	case DIR::UP:

		switch (searchDir)
		{
		case DIR::UP:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:
			if(CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}
		
			if(CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if(CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);	
			}

			if(CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
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
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;

		case DIR::UP_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
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
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_R:
			if (CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
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
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::UP_L:
			if (CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}
			break;

		case DIR::DOWN_L:
			if (CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			}

			if (CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			}
			break;
		}
		break;

	case DIR::UP_R:

		if(CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);	
		}

		if(CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);	
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
		}

		break;

	case DIR::DOWN_R:
		
		if(CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
		}

		break;

	case DIR::DOWN_L:

		if(CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
		}

		break;

	case DIR::UP_L:

		if(CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);	
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);	
		}

		if(CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);		
		}

		if(CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
		}

		if(CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir))
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
		}

		break;

	case DIR::NONE:

		for (int i = (int)DIR::UP; i < (int)DIR::NONE; i++)
		{
			if(CheckCorner(pCurNode->_pos, (DIR)i, newPos, newSearchDir))
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, (DIR)i, newSearchDir, newG);
			}
		}
		break;
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
			printf("UP: Obstacle (%d, %d)\n", up._x, up._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
		{
			newPos = up;
			searchDir = DIR::NONE;
			printf("★★★ UP: Dest (%d, %d)\n", up._x, up._y);
			return true;
		}

		if (up._y < 0)
		{
			printf("UP: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(up._x, up._y));

		while (up_r_front._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(up_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_r_front) != Map::OBSTACLE)
			{
				newPos = up;
				searchDir = DIR::UP_L;
			}

			if (_pNodeMgr->_pMap->GetMapState(up_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP_L)
				{
					searchDir == DIR::UP_R;
				}
				else
				{
					newPos = up;
					searchDir = DIR::UP;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (searchDir == DIR::UP)
					printf("★★★ UP: R L Corner (%d, %d)\n", up._x, up._y);
				else if (searchDir == DIR::UP_R)
					printf("★★★ UP: L Corner (%d, %d)\n", up._x, up._y);
				else if (searchDir == DIR::UP_L)
					printf("★★★ UP: R Corner (%d, %d)\n", up._x, up._y);

				return true;
			}

			up = up + _direction[(int)dir];
			up_r = up + _direction[(int)DIR::R];
			up_l = up + _direction[(int)DIR::L];
			up_r_front = up_r + _direction[(int)dir];
			up_l_front = up_l + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
			{
				printf("UP: Obstacle (%d, %d)\n", up._x, up._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
			{
				newPos = up;
				searchDir = DIR::NONE;
				printf("★★★ UP: Dest (%d, %d)\n", up._x, up._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(up._x, up._y));
		}

		printf("UP: Range Out\n");
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
			printf("DOWN: Obstacle (%d, %d)\n", down._x, down._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
		{
			newPos = down;
			searchDir = DIR::NONE;
			printf("★★★ DOWN: Dest (%d, %d)\n", down._x, down._y);
			return true;
		}

		if (down._y >= Y_MAX)
		{
			printf("DOWN: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(down._x, down._y));

		while (down_r_front._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(down_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_r_front) != Map::OBSTACLE)
			{
				newPos = down;
				searchDir = DIR::DOWN_L;
			}

			if (_pNodeMgr->_pMap->GetMapState(down_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::DOWN_L)
				{
					searchDir = DIR::DOWN;
				}
				else
				{
					newPos = down;
					searchDir = DIR::DOWN_R;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (searchDir == DIR::DOWN)
					printf("★★★ DOWN: R L Corner (%d, %d)\n", down._x, down._y);
				else if (searchDir == DIR::DOWN_L)
					printf("★★★ DOWN: R Corner (%d, %d)\n", down._x, down._y);
				else if (searchDir == DIR::DOWN_R)
					printf("★★★ DOWN: L Co (%d, %d)\n", down._x, down._y);

				return true;
			}

			down = down + _direction[(int)dir];
			down_r = down + _direction[(int)DIR::R];
			down_l = down + _direction[(int)DIR::L];
			down_r_front = down_r + _direction[(int)dir];
			down_l_front = down_l + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
			{
				printf("DOWN: Obstacle (%d, %d)\n", down._x, down._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
			{
				newPos = down;
				searchDir = DIR::NONE;
				printf("★★★ DOWN: Dest (%d, %d)\n", down._x, down._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(down._x, down._y));
		}

		printf("DOWN: Range Out\n");
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
			printf("R: Obstacle (%d, %d)\n", r._x, r._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
		{
			newPos = r;
			searchDir = DIR::NONE;
			printf("★★★ R: Dest (%d, %d)\n", r._x, r._y);
			return true;
		}

		if (r._x >= X_MAX)
		{
			printf("R: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(r._x, r._y));

		while (r_up_front._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(r_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_up_front) != Map::OBSTACLE)
			{
				newPos = r;
				searchDir = DIR::DOWN_R;
			}

			if (_pNodeMgr->_pMap->GetMapState(r_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::DOWN_R)
				{
					searchDir = DIR::R;
				}
				else
				{
					newPos = r;
					searchDir = DIR::UP_R;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (searchDir == DIR::R)
					printf("★★★ R: Up Down Corner (%d, %d)\n", r._x, r._y);
				else if (searchDir == DIR::DOWN_R)
					printf("★★★ R: UP Corner (%d, %d)\n", r._x, r._y);
				else if (searchDir == DIR::UP_R)
					printf("★★★ R: Down Corner (%d, %d)\n", r._x, r._y);

				return true;
			}

			r = r + _direction[(int)dir];
			r_up = r + _direction[(int)DIR::UP];
			r_down = r + _direction[(int)DIR::DOWN];
			r_up_front = r_up + _direction[(int)dir];
			r_down_front = r_down + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
			{
				printf("R: Obstacle (%d, %d)\n", r._x, r._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
			{
				newPos = r;
				searchDir = DIR::NONE;
				printf("★★★ R: Dest (%d, %d)\n", r._x, r._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(r._x, r._y));
		}

		printf("R: Range Out\n");
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
			printf("L: Obstacle (%d, %d)\n", l._x, l._y);
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
		{
			newPos = l;
			searchDir = DIR::NONE;
			printf("★★★ L: Dest (%d, %d)\n", l._x, l._y);
			return true;
		}

		if (l._x < 0)
		{
			printf("L: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedList.push_back(Pos(l._x, l._y));

		while (l_up_front._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(l_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_up_front) != Map::OBSTACLE)
			{
				newPos = l;
				searchDir = DIR::DOWN_L;
			}

			if (_pNodeMgr->_pMap->GetMapState(l_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::DOWN_L)
				{
					searchDir = DIR::L;
				}
				else
				{
					newPos = l;
					searchDir = DIR::UP_L;
				}
			}

			if (searchDir != DIR::NONE)
			{
				if (searchDir == DIR::L)
					printf("★★★ L: Up Down Corner (%d, %d)\n", l._x, l._y);
				else if (searchDir == DIR::DOWN_L)
					printf("★★★ L: Up Corner (%d, %d)\n", l._x, l._y);
				else if (searchDir == DIR::UP_L)
					printf("★★★ L: Down Corner (%d, %d)\n", l._x, l._y);

				return true;
			}

			l = l + _direction[(int)dir];
			l_up = l + _direction[(int)DIR::UP];
			l_down = l + _direction[(int)DIR::DOWN];
			l_up_front = l_up + _direction[(int)dir];
			l_down_front = l_down + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
			{
				printf("L: Obstacle (%d, %d)\n", l._x, l._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
			{
				newPos = l;
				searchDir = DIR::NONE;
				printf("★★★ L: Dest (%d, %d)\n", l._x, l._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(l._x, l._y));
		}

		printf("L: Range Out\n");
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
				printf("UP_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("★★★ UP_R: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::UP_R;
				printf("★★★ UP_R: UP_R (%d, %d)\n", diag._x, diag._y);
				return true;
			}
			
			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::UP_R, DIR::R, newPos, searchDir)) return true;
			if (CheckDiagCorner(diag, DIR::UP_R, DIR::UP, newPos, searchDir)) return true;

			diag = diag + _direction[(int)dir];
			diag_l = diag + _direction[(int)DIR::L];
			diag_down = diag + _direction[(int)DIR::DOWN];
		}

		printf("UP_R: Range Out\n");
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
				printf("DOWN_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("★★★ DOWN_R: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::DOWN_R;
				printf("★★★ DOWN_R: DOWN_R (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::DOWN_R, DIR::R, newPos, searchDir)) return true;
			if (CheckDiagCorner(diag, DIR::DOWN_R, DIR::DOWN, newPos, searchDir)) return true;

			diag = diag + _direction[(int)dir];
			diag_l = diag + _direction[(int)DIR::L];
			diag_up = diag + _direction[(int)DIR::UP];

		}
		printf("DOWN_R: Range Out\n");
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
				printf("DOWN_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("★★★ DOWN_L: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::DOWN_L;
				printf("★★★ DOWN_L: DOWN_L (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::DOWN_L, DIR::L, newPos, searchDir)) return true;
			if (CheckDiagCorner(diag, DIR::DOWN_L, DIR::DOWN, newPos, searchDir)) return true;

			diag = diag + _direction[(int)dir];
			diag_r = diag + _direction[(int)DIR::R];
			diag_up = diag + _direction[(int)DIR::UP];
		}
		printf("DOWN_L: Range Out\n");
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
				printf("UP_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("★★★ UP_L: Dest (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::UP_L;
				printf("★★★ UP_L: UP_L (%d, %d)\n", diag._x, diag._y);
				return true;
			}

			_pNodeMgr->_checkedList.push_back(Pos(diag._x, diag._y));

			if (CheckDiagCorner(diag, DIR::UP_L, DIR::L, newPos, searchDir)) return true;
			if (CheckDiagCorner(diag, DIR::UP_L, DIR::UP, newPos, searchDir)) return true;

			diag = diag + _direction[(int)dir];
			diag_r = diag + _direction[(int)DIR::R];
			diag_down = diag + _direction[(int)DIR::DOWN];
		}
		printf("UP_L: Range Out\n");
		return false;
	}
	break;
	}
}

bool JumpPointSearch::CheckDiagCorner(Pos diag, DIR diagDir, DIR searchDir, Pos& newPos, DIR& newSearchDir)
{
	Pos checkDir;

	switch (searchDir)
	{
	case DIR::R:
	{
		if(diagDir == DIR::UP_R)
			checkDir = _direction[(int)DIR::DOWN];
		else if (diagDir == DIR::DOWN_R)
			checkDir = _direction[(int)DIR::UP];

		Pos diag_r = diag;
		Pos diag_r_side = diag_r + checkDir;
		Pos diag_r_side_front = diag_r_side + _direction[(int)searchDir];

		if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::R;
			printf("★★★ DIAG: R, Dest (%d, %d) (%d, %d)\n",
				diag._x, diag._y, diag_r._x, diag_r._y);
			return true;
		}

		if (diag_r._x >= X_MAX)
		{
			printf("DIAG: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(diag_r._x, diag_r._y));

		while (diag_r_side_front._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(diag_r_side) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_r_side_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::R;
				printf("★★★ DIAG: R, Side (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_r_side._x, diag_r_side._y);
				_pNodeMgr->_diagCuzList.push_back(Pos(diag_r._x, diag_r._y));
				return true;
			}

			diag_r = diag_r + _direction[(int)searchDir];
			diag_r_side = diag_r + checkDir;
			diag_r_side_front = diag_r_side + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::R;
				printf("★★★ DIAG: R, Dest (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_r._x, diag_r._y);
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(diag_r._x, diag_r._y));
		}
		return false;
	}
	break;

	case DIR::UP:
	{
		if (diagDir == DIR::UP_R)
			checkDir = _direction[(int)DIR::L];
		else if (diagDir == DIR::UP_L)
			checkDir = _direction[(int)DIR::R];

		Pos diag_up = diag;
		Pos diag_up_side = diag_up + checkDir;
		Pos diag_up_side_front = diag_up_side + _direction[(int)searchDir];

		if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::UP;
			printf("★★★ DIAG: UP, Dest (%d, %d) (%d, %d)\n",
				diag._x, diag._y, diag_up._x, diag_up._y);
			return true;
		}

		if (diag_up._y < 0)
		{
			printf("DIAG: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(diag_up._x, diag_up._y));

		while (diag_up_side_front._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(diag_up_side) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up_side_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::UP;
				printf("★★★ DIAG: UP, Side (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_up_side._x, diag_up_side._y);
				_pNodeMgr->_diagCuzList.push_back(Pos(diag_up._x, diag_up._y));
				return true;
			}

			diag_up = diag_up + _direction[(int)searchDir];
			diag_up_side = diag_up + checkDir;
			diag_up_side_front = diag_up_side + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::UP;
				printf("★★★ DIAG: UP, Dest (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_up._x, diag_up._y);
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(diag_up._x, diag_up._y));
		}
		return false;
	}
		break;

	case DIR::L:
	{
		if (diagDir == DIR::UP_L)
			checkDir = _direction[(int)DIR::DOWN];
		else if (diagDir == DIR::DOWN_L)
			checkDir = _direction[(int)DIR::UP];

		Pos diag_l = diag;
		Pos diag_l_side = diag_l + checkDir;
		Pos diag_l_side_front = diag_l_side + _direction[(int)searchDir];

		if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::L;
			printf("★★★ DIAG: L, Dest (%d, %d) (%d, %d)\n",
				diag._x, diag._y, diag_l._x, diag_l._y);
			return true;
		}

		if (diag_l._x < 0)
		{
			printf("DIAG: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(diag_l._x, diag_l._y));

		while (diag_l_side_front._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(diag_l_side) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_l_side_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::L;
				printf("★★★ DIAG: L, Up (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_l_side._x, diag_l_side._y);
				_pNodeMgr->_diagCuzList.push_back(Pos(diag_l._x, diag_l._y));
				return true;
			}

			diag_l = diag_l + _direction[(int)searchDir];
			diag_l_side = diag_l + checkDir;
			diag_l_side_front = diag_l_side + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::L;
				printf("★★★ DIAG: L, Dest (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_l._x, diag_l._y);
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(diag_l._x, diag_l._y));
		}

		return false;
	}
		break;
	
	case DIR::DOWN:
	{
		if (diagDir == DIR::DOWN_R)
			checkDir = _direction[(int)DIR::L];
		else if (diagDir == DIR::DOWN_L)
			checkDir = _direction[(int)DIR::R];

		Pos diag_down = diag;
		Pos diag_down_side = diag_down + checkDir;
		Pos diag_down_side_front = diag_down_side + _direction[(int)searchDir];

		if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
		{
			return false;
		}

		if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::DEST)
		{
			newPos = diag;
			newSearchDir = DIR::DOWN;
			printf("★★★ DIAG: Down, Dest (%d, %d) (%d, %d)\n",
				diag._x, diag._y, diag_down._x, diag_down._y);
			return true;
		}

		if (diag_down._y >= Y_MAX)
		{
			printf("DIAG: Range Out\n");
			return false;
		}

		_pNodeMgr->_checkedDiagList.push_back(Pos(diag_down._x, diag_down._y));

		while (diag_down_side_front._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(diag_down_side) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down_side_front) != Map::OBSTACLE)
			{
				newPos = diag;
				newSearchDir = DIR::DOWN;
				printf("★★★ DIAG: Down, R (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_down_side._x, diag_down_side._y);
				_pNodeMgr->_diagCuzList.push_back(Pos(diag_down._x, diag_down._y));
				return true;
			}

			diag_down = diag_down + _direction[(int)searchDir];
			diag_down_side = diag_down + checkDir;
			diag_down_side_front = diag_down_side + _direction[(int)searchDir];

			if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
			{
				return false;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::DEST)
			{
				newPos = diag;
				newSearchDir = DIR::DOWN;
				printf("★★★ DIAG: Down, Dest (%d, %d) (%d, %d)\n",
					diag._x, diag._y, diag_down._x, diag_down._y);
				return true;
			}

			_pNodeMgr->_checkedDiagList.push_back(Pos(diag_down._x, diag_down._y));
		}

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
			newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
			dir,
			searchDir,
			pCurNode
		);

		/*
		printf("Create New Node (%d, %d) (parent: %d, %d) (%d + %d = %d)\n",
			pNew->_pos._x, pNew->_pos._y,
			pNew->_pParent->_pos._x, pNew->_pParent->_pos._y,
			pNew->_g, pNew->_h, pNew->_f);
		*/

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
			//printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
			//	newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			//printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
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
			//printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
			//	newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			//printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
		}
	}
	break;

	case Map::OBSTACLE:
	case Map::RANGE_OUT:
		//printf("Can't Go (%d, %d)\n", newPos._x, newPos._y);
		break;

	}
}

void JumpPointSearch::PrintOpenListForDebug()
{
	printf("\n=====================================\n");
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

	printf("\n=====================================\n");
}


bool JumpPointSearch::CompareG::operator()(Node*& pNode) const
{
	return (pNode->_pos == pos && pNode->_g > g);
}