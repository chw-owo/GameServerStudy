#include "JumpPointSearch.h"
#include "Node.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
using namespace std;

void JumpPointSearch::FindPath()
{
	if (!_bFindPathOn) return;

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", 
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("Can't Find Path!\n");
			_bFindPathOn = false;
			return;
		}

		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();

		printf("\n");
	}
	else
	{
		printf("Complete Find Path\n=================================\n");
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
	}

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", 
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		CheckCreateNode(_pNodeMgr->_pCurNode);

		if (_pNodeMgr->_openList.empty())
		{
			printf("Can't Find Path!\n");
			_bFindPathStepOn = false;
			return;
		}

		_pNodeMgr->_closeList.push_back(_pNodeMgr->_pCurNode);
		_pNodeMgr->_pMap->SetMapState(
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y, Map::CLOSE);

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pNodeMgr->_pCurNode = _pNodeMgr->_openList.front();
		PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();
	}
	else
	{
		printf("Complete Find Path\n=================================\n");
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
		_bFindPathStepOn = false;
	}
}

void JumpPointSearch::CheckCreateNode(Node* pCurNode)
{
	int newG = 0;
	Pos newPos = Pos(-1, 0);
	DIR dir = pCurNode->_dir;
	DIR searchDir = pCurNode->_searchDir;
	DIR newSearchDir = DIR::NONE; 

	printf("\n(%d, %d): dir - %d, searchDir - %d\n",
		pCurNode->_pos._x, pCurNode->_pos._y, dir, searchDir );

	switch (dir)
	{
	case DIR::UP:

		CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			newPos._x = -1;
		}

		break;

	case DIR::R:

		CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			newPos._x = -1;
		}

		break;

	case DIR::DOWN:

		CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			newPos._x = -1;
		}
		break;

	case DIR::L:
		CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
			newPos._x = -1;
		}

		CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir);
		if (newPos._x != -1)
		{
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
			newPos._x = -1;
		}
		
		break;

	case DIR::UP_R:
		
		if (searchDir == DIR::UP_R)
		{
			CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::UP_R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_R, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::UP)
		{
			CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::R)
		{
			CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
				newPos._x = -1;
			}
		}
		break;

	case DIR::DOWN_R:

		if (searchDir == DIR::DOWN_R)
		{
			printf("CheckCreateNode: dir - DOWN_R, searchDir - DOWN_R\n");
			CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::DOWN_R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_R, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::DOWN)
		{
			printf("CheckCreateNode: dir - DOWN_R, searchDir - DOWN\n");
			CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::R)
		{
			printf("CheckCreateNode: dir - DOWN_R, searchDir - R\n");
			CheckCorner(pCurNode->_pos, DIR::R, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::R, newSearchDir, newG);
				newPos._x = -1;
			}
		}
		break;

	case DIR::DOWN_L:
		if (searchDir == DIR::DOWN_L)
		{
			printf("CheckCreateNode: dir - DOWN_L, searchDir - DOWN_L\n");
			CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::DOWN_L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN_L, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::DOWN)
		{
			printf("CheckCreateNode: dir - DOWN_L, searchDir - DOWN\n");
			CheckCorner(pCurNode->_pos, DIR::DOWN, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::DOWN, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::L)
		{
			printf("CheckCreateNode: dir - DOWN_L, searchDir - L\n");
			CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
				newPos._x = -1;
			}
		}
		break;

	case DIR::UP_L:
		if (searchDir == DIR::UP_L)
		{
			printf("CheckCreateNode: dir - UP_L, searchDir - UP_L\n");
			CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::UP_L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP_L, newSearchDir, newG);
				newPos._x = -1;
			}

			CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::UP)
		{
			printf("CheckCreateNode: dir - UP_L, searchDir - UP\n");
			CheckCorner(pCurNode->_pos, DIR::UP, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::UP, newSearchDir, newG);
				newPos._x = -1;
			}
		}

		else if (searchDir == DIR::L)
		{
			printf("CheckCreateNode: dir - UP_L, searchDir - L\n");
			CheckCorner(pCurNode->_pos, DIR::L, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, DIR::L, newSearchDir, newG);
				newPos._x = -1;
			}
		}
		break;

	case DIR::NONE:
		printf("CheckCreateNode: dir - NONE\n");
		for (int i = (int)DIR::UP; i < (int)DIR::NONE; i++)
		{
			CheckCorner(pCurNode->_pos, (DIR)i, newPos, newSearchDir);
			if (newPos._x != -1)
			{
				newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
				CreateNode(pCurNode, newPos, (DIR)i, newSearchDir, newG);
				newPos._x = -1;
			}
		}
		break;
	}	
}

void JumpPointSearch::CheckCorner(Pos curPos, DIR dir, Pos& newPos, DIR& searchDir)
{
	switch (dir)
	{
	case DIR::UP:
	{
		Pos up = curPos + _direction[(int)dir];
		Pos up_r = up + _direction[(int)DIR::R];
		Pos up_l = up + _direction[(int)DIR::L];
		Pos up_r_front = up_r + _direction[(int)dir];
		Pos up_l_front = up_l + _direction[(int)dir];

		while (up._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE)
			{
				printf("UP: Obstacle (%d, %d)\n", up._x, up._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(up) == Map::DEST)
			{
				newPos = up;
				searchDir = DIR::NONE;
				printf("UP: Dest (%d, %d)\n", up._x, up._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(up_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_r_front) != Map::OBSTACLE)
			{
				newPos = up;
				searchDir = DIR::R;
				printf("UP: R (%d, %d)\n", up._x, up._y);
			}

			if (_pNodeMgr->_pMap->GetMapState(up_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::R)
				{
					searchDir == DIR::R_L;
				}
				else
				{
					newPos = up;
					searchDir = DIR::L;
				}
				printf("UP: L (%d, %d)\n", up._x, up._y);
			}

			if (searchDir != DIR::NONE)
				return;

			up = up + _direction[(int)dir];
			up_r = up + _direction[(int)DIR::R];
			up_l = up + _direction[(int)DIR::L];
			up_r_front = up_r + _direction[(int)dir];
			up_l_front = up_l + _direction[(int)dir];
		}
	}
	break;

	case DIR::DOWN:
	{
		Pos down = curPos + _direction[(int)dir];
		Pos down_r = down + _direction[(int)DIR::R];
		Pos down_l = down + _direction[(int)DIR::L];
		Pos down_r_front = down_r + _direction[(int)dir];
		Pos down_l_front = down_l + _direction[(int)dir];

		while (down._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE)
			{
				printf("DOWN: Obstacle (%d, %d)\n", down._x, down._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::DEST)
			{
				newPos = down;
				searchDir = DIR::NONE;
				printf("DOWN: Dest (%d, %d)\n", down._x, down._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(down_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_r_front) != Map::OBSTACLE)
			{
				newPos = down;
				searchDir = DIR::R;
				printf("DOWN: R (%d, %d)\n", down._x, down._y);
			}

			if (_pNodeMgr->_pMap->GetMapState(down_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_l_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::R)
				{
					searchDir = DIR::R_L;
				}
				else
				{
					newPos = down;
					searchDir = DIR::L;
				}
				printf("DOWN: L (%d, %d)\n", down._x, down._y);
			}

			if (searchDir != DIR::NONE)
				return;

			down = down + _direction[(int)dir];
			down_r = down + _direction[(int)DIR::R];
			down_l = down + _direction[(int)DIR::L];
			down_r_front = down_r + _direction[(int)dir];
			down_l_front = down_l + _direction[(int)dir];
		}
	}
	break;

	case DIR::R:
	{
		Pos r = curPos + _direction[(int)dir];
		Pos r_up = r + _direction[(int)DIR::UP];
		Pos r_down = r + _direction[(int)DIR::DOWN];
		Pos r_up_front = r_up + _direction[(int)dir];
		Pos r_down_front = r_down + _direction[(int)dir];

		while (r._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE)
			{
				printf("R: Obstacle (%d, %d)\n", r._x, r._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(r) == Map::DEST)
			{
				newPos = r;
				searchDir = DIR::NONE;
				printf("R: Dest (%d, %d)\n", r._x, r._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(r_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_up_front) != Map::OBSTACLE)
			{
				newPos = r;
				searchDir = DIR::UP;
				printf("R: Up (%d, %d)\n", r._x, r._y);
			}

			if (_pNodeMgr->_pMap->GetMapState(r_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP)
				{
					searchDir = DIR::UP_DOWN;
				}
				else
				{
					newPos = r;
					searchDir = DIR::DOWN;
				}
				printf("R: Down (%d, %d)\n", r._x, r._y);
			}

			if (searchDir != DIR::NONE)
				return;

			r = r + _direction[(int)dir];
			r_up = r + _direction[(int)DIR::UP];
			r_down = r + _direction[(int)DIR::DOWN];
			r_up_front = r_up + _direction[(int)dir];
			r_down_front = r_down + _direction[(int)dir];
		}
	}
	break;

	case DIR::L:
	{
		Pos l = curPos + _direction[(int)dir];
		Pos l_up = l + _direction[(int)DIR::UP];
		Pos l_down = l + _direction[(int)DIR::DOWN];
		Pos l_up_front = l_up + _direction[(int)dir];
		Pos l_down_front = l_down + _direction[(int)dir];

		while (l._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE)
			{
				printf("L: Obstacle (%d, %d)\n", l._x, l._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::DEST)
			{
				newPos = l;
				searchDir = DIR::NONE;
				printf("L: Dest (%d, %d)\n", l._x, l._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(l_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_up_front) != Map::OBSTACLE)
			{
				newPos = l;
				searchDir = DIR::UP;
				printf("L: Up (%d, %d)\n", l._x, l._y);
			}

			if (_pNodeMgr->_pMap->GetMapState(l_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_down_front) != Map::OBSTACLE)
			{
				if (searchDir == DIR::UP)
				{
					searchDir = DIR::UP_DOWN;
				}
				else
				{
					newPos = l;
					searchDir = DIR::DOWN;
				}

				printf("L: Down (%d, %d)\n", l._x, l._y);
			}

			if (searchDir != DIR::NONE)
				return;

			l = l + _direction[(int)dir];
			l_up = l + _direction[(int)DIR::UP];
			l_down = l + _direction[(int)DIR::DOWN];
			l_up_front = l_up + _direction[(int)dir];
			l_down_front = l_down + _direction[(int)dir];
		}
	}
	break;

	case DIR::UP_R:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_down = diag + _direction[(int)DIR::DOWN];
		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_upl = diag + _direction[(int)DIR::UP_L];
		Pos diag_downr = diag + _direction[(int)DIR::DOWN_R];

		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_r_up = diag_r + _direction[(int)DIR::UP];
		Pos diag_r_down = diag_r + _direction[(int)DIR::DOWN];
		Pos diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
		Pos diag_r_down_front = diag_r_down + _direction[(int)DIR::R];

		Pos diag_up = diag + _direction[(int)DIR::UP];
		Pos diag_up_r = diag_up + _direction[(int)DIR::R];
		Pos diag_up_l = diag_up + _direction[(int)DIR::L];
		Pos diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
		Pos diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];

		while (diag._x < X_MAX && diag._y >= 0)
		{
			// �밢�� ���� üũ
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				printf("UP_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("UP_R: Dest (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upl) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downr) != Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::UP_R;
				printf("UP_R: UP_R (%d, %d)\n", diag._x, diag._y);
				return;
			}
			

			// ������ ���� üũ
			while (diag_r._x < X_MAX)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("UP_R: R, Dest (%d, %d) (%d, %d)\n", 
						diag._x, diag._y, diag_r._x, diag_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r_up) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_r_up_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("UP_R: R, Up (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_r_up._x, diag_r_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r_down) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_r_down_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("UP_R: R, Down (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_r_down._x, diag_r_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				diag_r = diag_r + _direction[(int)DIR::R];
				diag_r_up = diag_r + _direction[(int)DIR::UP];
				diag_r_down = diag_r + _direction[(int)DIR::DOWN];
				diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
				diag_r_down_front = diag_r_down + _direction[(int)DIR::R];
			}

			// ���� ���� üũ
			while (diag_up._y >= 0)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_R: UP, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up._x, diag_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up_r) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_up_r_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_R: UP, R (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up_r._x, diag_up_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up_l) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_up_l_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_R: UP, L (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up_l._x, diag_up_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				diag_up = diag_up + _direction[(int)DIR::UP];
				diag_up_r = diag_up + _direction[(int)DIR::R];
				diag_up_l = diag_up + _direction[(int)DIR::L];
				diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
				diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];
			}

			diag = diag + _direction[(int)dir];

			diag_down = diag + _direction[(int)DIR::DOWN];
			diag_l = diag + _direction[(int)DIR::L];
			diag_upl = diag + _direction[(int)DIR::UP_L];
			diag_downr = diag + _direction[(int)DIR::DOWN_R];

			diag_r = diag + _direction[(int)DIR::R];
			diag_r_up = diag_r + _direction[(int)DIR::UP];
			diag_r_down = diag_r + _direction[(int)DIR::DOWN];
			diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
			diag_r_down_front = diag_r_down + _direction[(int)DIR::R];

			diag_up = diag + _direction[(int)DIR::UP];
			diag_up_r = diag_up + _direction[(int)DIR::R];
			diag_up_l = diag_up + _direction[(int)DIR::L];
			diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
			diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];
		}
	}
	break;

	case DIR::DOWN_R:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_up = diag + _direction[(int)DIR::UP];
		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_upr = diag + _direction[(int)DIR::UP_R];
		Pos diag_downl = diag + _direction[(int)DIR::DOWN_L];

		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_r_up = diag_r + _direction[(int)DIR::UP];
		Pos diag_r_down = diag_r + _direction[(int)DIR::DOWN];
		Pos diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
		Pos diag_r_down_front = diag_r_down + _direction[(int)DIR::R];

		Pos diag_down = diag + _direction[(int)DIR::DOWN];
		Pos diag_down_r = diag_down + _direction[(int)DIR::R];
		Pos diag_down_l = diag_down + _direction[(int)DIR::L];
		Pos diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
		Pos diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];

		while (diag._x < X_MAX && diag._y < Y_MAX)
		{
			// �밢�� ���� üũ
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				printf("DOWN_R: Obstacle (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("DOWN_R: Dest (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upr) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downl) != Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::DOWN_R;
				printf("DOWN_R: DOWN_R (%d, %d)\n", diag._x, diag._y);
				return;
			}

			// ������ ���� üũ
			while (diag_r._x < X_MAX)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("DOWN_R: R, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_r._x, diag_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r_up) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_r_up_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("DOWN_R: R, Up (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_r_up._x, diag_r_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_r_down) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_r_down_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::R;
					printf("DOWN_R: R, Down (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_r_down._x, diag_r_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_r._x, diag_r._y, Map::DIAG_CUZ);
					return;
				}

				diag_r = diag_r + _direction[(int)DIR::R];
				diag_r_up = diag_r + _direction[(int)DIR::UP];
				diag_r_down = diag_r + _direction[(int)DIR::DOWN];
				diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
				diag_r_down_front = diag_r_down + _direction[(int)DIR::R];
			}

			// �Ʒ��� ���� üũ
			while (diag_down._y < Y_MAX)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_R: DOWN, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down._x, diag_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down_r) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_down_r_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_R: DOWN, R (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down_r._x, diag_down_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down_l) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_down_l_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_R: DOWN, L (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down_l._x, diag_down_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				diag_down = diag_down + _direction[(int)DIR::DOWN];
				diag_down_r = diag_down + _direction[(int)DIR::R];
				diag_down_l = diag_down + _direction[(int)DIR::L];
				diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
				diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];
			}

			diag = diag + _direction[(int)dir];

			diag_up = diag + _direction[(int)DIR::UP];
			diag_l = diag + _direction[(int)DIR::L];
			diag_upr = diag + _direction[(int)DIR::UP_R];
			diag_downl = diag + _direction[(int)DIR::DOWN_L];

			diag_r = diag + _direction[(int)DIR::R];
			diag_r_up = diag_r + _direction[(int)DIR::UP];
			diag_r_down = diag_r + _direction[(int)DIR::DOWN];
			diag_r_up_front = diag_r_up + _direction[(int)DIR::R];
			diag_r_down_front = diag_r_down + _direction[(int)DIR::R];

			diag_down = diag + _direction[(int)DIR::DOWN];
			diag_down_r = diag_down + _direction[(int)DIR::R];
			diag_down_l = diag_down + _direction[(int)DIR::L];
			diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
			diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];
		}
	}
	break;

	case DIR::DOWN_L:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_up = diag + _direction[(int)DIR::UP];
		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_upl = diag + _direction[(int)DIR::UP_L];
		Pos diag_downr = diag + _direction[(int)DIR::DOWN_R];

		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_l_up = diag_l + _direction[(int)DIR::UP];
		Pos diag_l_down = diag_l + _direction[(int)DIR::DOWN];
		Pos diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
		Pos diag_l_down_front = diag_l_down + _direction[(int)DIR::L];

		Pos diag_down = diag + _direction[(int)DIR::DOWN];
		Pos diag_down_r = diag_down + _direction[(int)DIR::R];
		Pos diag_down_l = diag_down + _direction[(int)DIR::L];
		Pos diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
		Pos diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];

		while (diag._x >= 0 && diag._y < Y_MAX)
		{
			// �밢�� ���� üũ
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				printf("DOWN_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("DOWN_L: Dest (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upl) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downr) != Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::DOWN_L;
				printf("DOWN_L: DOWN_L (%d, %d)\n", diag._x, diag._y);
				return;
			}

			// ���� ���� üũ
			while (diag_l._x >= 0)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("DOWN_L: L, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l._x, diag_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l_up) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_l_up_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("DOWN_L: L, Up (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l_up._x, diag_l_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l_down) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_l_down_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("DOWN_L: L, Down (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l_down._x, diag_l_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				diag_l = diag_l + _direction[(int)DIR::L];
				diag_l_up = diag_l + _direction[(int)DIR::UP];
				diag_l_down = diag_l + _direction[(int)DIR::DOWN];
				diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
				diag_l_down_front = diag_l_down + _direction[(int)DIR::L];
			}

			// �Ʒ��� ���� üũ
			while (diag_down._y < Y_MAX)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_L: Down, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down._x, diag_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down_r) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_down_r_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_L: Down, R (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down_r._x, diag_down_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_down_l) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_down_l_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::DOWN;
					printf("DOWN_L: Down, L (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_down_l._x, diag_down_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_down._x, diag_down._y, Map::DIAG_CUZ);
					return;
				}

				diag_down = diag_down + _direction[(int)DIR::DOWN];
				diag_down_r = diag_down + _direction[(int)DIR::R];
				diag_down_l = diag_down + _direction[(int)DIR::L];
				diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
				diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];
			}

			diag = diag + _direction[(int)dir];

			diag_up = diag + _direction[(int)DIR::UP];
			diag_r = diag + _direction[(int)DIR::R];
			diag_upl = diag + _direction[(int)DIR::UP_L];
			diag_downr = diag + _direction[(int)DIR::DOWN_R];

			diag_l = diag + _direction[(int)DIR::L];
			diag_l_up = diag_l + _direction[(int)DIR::UP];
			diag_l_down = diag_l + _direction[(int)DIR::DOWN];
			diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
			diag_l_down_front = diag_l_down + _direction[(int)DIR::L];

			diag_down = diag + _direction[(int)DIR::DOWN];
			diag_down_r = diag_down + _direction[(int)DIR::R];
			diag_down_l = diag_down + _direction[(int)DIR::L];
			diag_down_r_front = diag_down_r + _direction[(int)DIR::DOWN];
			diag_down_l_front = diag_down_l + _direction[(int)DIR::DOWN];
		}
	}
	break;

	case DIR::UP_L:
	{
		Pos diag = curPos + _direction[(int)dir];
		Pos diag_down = diag + _direction[(int)DIR::DOWN];
		Pos diag_r = diag + _direction[(int)DIR::R];
		Pos diag_upr = diag + _direction[(int)DIR::UP_R];
		Pos diag_downl = diag + _direction[(int)DIR::DOWN_L];

		Pos diag_l = diag + _direction[(int)DIR::L];
		Pos diag_l_up = diag_l + _direction[(int)DIR::UP];
		Pos diag_l_down = diag_l + _direction[(int)DIR::DOWN];
		Pos diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
		Pos diag_l_down_front = diag_l_down + _direction[(int)DIR::L];

		Pos diag_up = diag + _direction[(int)DIR::UP];
		Pos diag_up_r = diag_up + _direction[(int)DIR::R];
		Pos diag_up_l = diag_up + _direction[(int)DIR::L];
		Pos diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
		Pos diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];

		while (diag._x >= 0 && diag._y >= 0)
		{
			// �밢�� ���� üũ
			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::OBSTACLE)
			{
				printf("UP_L: Obstacle (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag) == Map::DEST)
			{
				newPos = diag;
				searchDir = DIR::NONE;
				printf("UP_L: Dest (%d, %d)\n", diag._x, diag._y);
				return;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upr) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downl) != Map::OBSTACLE)
			{
				newPos = diag;
				searchDir = DIR::UP_L;
				printf("UP_L: UP_L (%d, %d)\n", diag._x, diag._y);
				return;
			}


			// ���� ���� üũ
			while (diag_l._x >= 0)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("UP_L: L, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l._x, diag_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l_up) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_l_up_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("UP_L: L, Up (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l_up._x, diag_l_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_l_down) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_l_down_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::L;
					printf("UP_L: L, Down (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_l_down._x, diag_l_down._y);
					_pNodeMgr->_pMap->SetMapState(diag_l._x, diag_l._y, Map::DIAG_CUZ);
					return;
				}

				diag_l = diag_l + _direction[(int)DIR::L];
				diag_l_up = diag_l + _direction[(int)DIR::UP];
				diag_l_down = diag_l + _direction[(int)DIR::DOWN];
				diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
				diag_l_down_front = diag_l_down + _direction[(int)DIR::L];
			}

			// ���� ���� üũ
			while (diag_up._y >= 0)
			{
				if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE)
				{
					break;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up) == Map::DEST)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_L: Up, Dest (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up._x, diag_up._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up_r) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_up_r_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_L: Up, R (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up_r._x, diag_up_r._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				if (_pNodeMgr->_pMap->GetMapState(diag_up_l) == Map::OBSTACLE &&
					_pNodeMgr->_pMap->GetMapState(diag_up_l_front) != Map::OBSTACLE)
				{
					newPos = diag;
					searchDir = DIR::UP;
					printf("UP_L: Up, L (%d, %d) (%d, %d)\n",
						diag._x, diag._y, diag_up_l._x, diag_up_l._y);
					_pNodeMgr->_pMap->SetMapState(diag_up._x, diag_up._y, Map::DIAG_CUZ);
					return;
				}

				diag_up = diag_up + _direction[(int)DIR::UP];
				diag_up_r = diag_up + _direction[(int)DIR::R];
				diag_up_l = diag_up + _direction[(int)DIR::L];
				diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
				diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];
			}

			diag = diag + _direction[(int)dir];

			diag_down = diag + _direction[(int)DIR::DOWN];
			diag_r = diag + _direction[(int)DIR::R];
			diag_upr = diag + _direction[(int)DIR::UP_R];
			diag_downl = diag + _direction[(int)DIR::DOWN_L];

			diag_l = diag + _direction[(int)DIR::L];
			diag_l_up = diag_l + _direction[(int)DIR::UP];
			diag_l_down = diag_l + _direction[(int)DIR::DOWN];
			diag_l_up_front = diag_l_up + _direction[(int)DIR::L];
			diag_l_down_front = diag_l_down + _direction[(int)DIR::L];

			diag_up = diag + _direction[(int)DIR::UP];
			diag_up_r = diag_up + _direction[(int)DIR::R];
			diag_up_l = diag_up + _direction[(int)DIR::L];
			diag_up_r_front = diag_up_r + _direction[(int)DIR::UP];
			diag_up_l_front = diag_up_l + _direction[(int)DIR::UP];
		}
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
	case Map::DIAG_CUZ:
	{
		Node* pNew = new Node(
			newPos,
			newG,
			newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
			dir,
			searchDir,
			pCurNode
		);

		printf("Create New Node (%d, %d) (parent: %d, %d) (%d + %d = %d)\n",
			pNew->_pos._x, pNew->_pos._y,
			pNew->_pParent->_pos._x, pNew->_pParent->_pos._y,
			pNew->_g, pNew->_h, pNew->_f);

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
			printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
				newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
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
			printf("Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
				newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
		}
		else
		{
			printf("Already Exist (%d, %d)\n", newPos._x, newPos._y);
		}
	}
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