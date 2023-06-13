#include "JumpPointSearch.h"
#include "Node.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
using namespace std;

void JumpPointSearch::FindPath()
{
	if (!_bOn) return;

	if (_pCurNode == nullptr)
		_pCurNode = _pNodeMgr->_pStart;

	if (_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
		CheckCreateNode(_pCurNode, _pCurNode->_dir);

		if (_pNodeMgr->_openList.empty())
		{
			printf("Can't Find Path!\n");
			_bOn = false;
			_pCurNode = nullptr;
			return;
		}

		_pNodeMgr->_closeList.push_back(_pCurNode);
		_pNodeMgr->_pMap->SetMapState(_pCurNode->_pos._x, _pCurNode->_pos._y, Map::CLOSE);

		make_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end(), compareF);
		_pCurNode = _pNodeMgr->_openList.front();
		//PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();
	}
	else
	{
		printf("Complete Find Path\n=================================\n");
		_bOn = false;
		_pNodeMgr->_pDest = _pCurNode;
		_pCurNode = nullptr;
	}
}

void JumpPointSearch::CheckCreateNode(Node* pCurNode, DIR dir)
{
	int newG = 0;
	Pos newPos = Pos(0, 0);
	
	switch (dir)
	{
	case DIR::UP:
		break;

	case DIR::R:
		break;

	case DIR::DOWN:
		break;

	case DIR::L:
		break;

	case DIR::UP_R:
		break;

	case DIR::DOWN_R:
		break;

	case DIR::DOWN_L:
		break;

	case DIR::UP_L:
		break;

	case DIR::NONE:
		
		for (int i = (int)DIR::UP; i < (int)DIR::NONE; i++)
		{
			CheckCorner(pCurNode->_pos, (DIR)i, newPos);
			newG = pCurNode->_g + pCurNode->_pos.GetDistance(newPos);
			CreateNode(pCurNode, newPos, (DIR)i, newG);
		}

		break;
	}	
}

void JumpPointSearch::CheckCorner(Pos curPos, DIR dir, Pos& newPos)
{
	switch (dir)
	{
	case DIR::UP:
	{
		Pos r = curPos + _direction[(int)DIR::R];
		Pos l = curPos + _direction[(int)DIR::L];
		Pos r_front = r + _direction[(int)dir];
		Pos l_front = l + _direction[(int)dir];

		while (r_front._y >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_front) != Map::OBSTACLE)
			{
				newPos = r + _direction[(int)DIR::L];
				break;
			}

			r = r_front;
			r_front = r_front + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_front) != Map::OBSTACLE)
			{
				newPos = l + _direction[(int)DIR::R];
				break;
			}

			l = l_front;
			l_front = l_front + _direction[(int)dir];
		}

		newPos = r + _direction[(int)DIR::L];
	}
	break;

	case DIR::DOWN:
	{
		Pos r = curPos + _direction[(int)DIR::R];
		Pos l = curPos + _direction[(int)DIR::L];
		Pos r_front = r + _direction[(int)dir];
		Pos l_front = l + _direction[(int)dir];

		while (r_front._y < Y_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(r_front) != Map::OBSTACLE)
			{
				newPos = r + _direction[(int)DIR::L];
				break;
			}

			r = r_front;
			r_front = r_front + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(l_front) != Map::OBSTACLE)
			{
				newPos = l + _direction[(int)DIR::R];
				break;
			}

			l = l_front;
			l_front = l_front + _direction[(int)dir];
		}

		newPos = r + _direction[(int)DIR::L];
	}
	break;

	case DIR::R:
	{
		Pos up = curPos + _direction[(int)DIR::UP];
		Pos down = curPos + _direction[(int)DIR::DOWN];
		Pos up_front = up + _direction[(int)dir];
		Pos down_front = down + _direction[(int)dir];

		while (up_front._x < X_MAX)
		{
			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_front) != Map::OBSTACLE)
			{
				newPos = up + _direction[(int)DIR::DOWN];
				break;
			}

			up = up_front;
			up_front = up_front + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_front) != Map::OBSTACLE)
			{
				newPos = down + _direction[(int)DIR::UP];
				break;
			}

			down = down_front;
			down_front = down_front + _direction[(int)dir];
		}

		newPos = up + _direction[(int)DIR::DOWN];
	}
	break;

	case DIR::L:
	{
		Pos up = curPos + _direction[(int)DIR::UP];
		Pos down = curPos + _direction[(int)DIR::DOWN];
		Pos up_front = up + _direction[(int)dir];
		Pos down_front = down + _direction[(int)dir];

		while (up_front._x >= 0)
		{
			if (_pNodeMgr->_pMap->GetMapState(up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(up_front) != Map::OBSTACLE)
			{
				newPos = up + _direction[(int)DIR::DOWN];
				break;
			}

			up = up_front;
			up_front = up_front + _direction[(int)dir];

			if (_pNodeMgr->_pMap->GetMapState(down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(down_front) != Map::OBSTACLE)
			{
				newPos = down + _direction[(int)DIR::UP];
				break;
			}

			down = down_front;
			down_front = down_front + _direction[(int)dir];
		}

		newPos = up + _direction[(int)DIR::DOWN];
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

		while (diag_downr._x < X_MAX && diag_upl._y >= 0)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upl) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downr) != Map::OBSTACLE)
			{
				break;
			}

			// 오른쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_r_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_r_up_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_r_down_front) != Map::OBSTACLE)
			{
				break;
			}

			// 위쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_up_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up_r_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_up_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up_l_front) != Map::OBSTACLE)
			{
				break;
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

		newPos = diag;
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

		while (diag_upr._x < X_MAX && diag_downl._y < Y_MAX)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upr) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downl) != Map::OBSTACLE)
			{
				break;
			}

			// 오른쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_r_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_r_up_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_r_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_r_down_front) != Map::OBSTACLE)
			{
				break;
			}

			// 아래쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_down_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down_r_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_down_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down_l_front) != Map::OBSTACLE)
			{
				break;
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

		newPos = diag;
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

		while (diag_upl._x >= 0 && diag_downr._y < Y_MAX)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upl) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downr) != Map::OBSTACLE)
			{
				break;
			}

			// 오른쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_l_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_l_up_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_l_down_front) != Map::OBSTACLE)
			{
				break;
			}

			// 위쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_down_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down_r_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_down_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_down_l_front) != Map::OBSTACLE)
			{
				break;
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

		newPos = diag;
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

		while (diag_downl._x >= 0 && diag_upr._y >= 0)
		{
			// 대각선 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_upr) != Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_downl) != Map::OBSTACLE)
			{
				break;
			}

			// 오른쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_l_up) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_l_up_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_l_down) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_l_down_front) != Map::OBSTACLE)
			{
				break;
			}

			// 아래쪽 방향 체크
			if (_pNodeMgr->_pMap->GetMapState(diag_up_r) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up_r_front) != Map::OBSTACLE)
			{
				break;
			}

			if (_pNodeMgr->_pMap->GetMapState(diag_up_l) == Map::OBSTACLE &&
				_pNodeMgr->_pMap->GetMapState(diag_up_l_front) != Map::OBSTACLE)
			{
				break;
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

		newPos = diag;
	}
	break;
	}

	
}

void JumpPointSearch::CreateNode(Node* pCurNode, Pos newPos, DIR dir, int newG)
{
	Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);
	switch (state)
	{
	case Map::EMPTY:
	case Map::START:
	case Map::DEST:
	{
		Node* pNew = new Node(
			newPos,
			newG,
			newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
			dir,
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
			(*it)->ResetParent(newG, dir, pCurNode);
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
			(*it)->ResetParent(newG, dir, pCurNode);
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
		_pCurNode->_pos._x,
		_pCurNode->_pos._y,
		_pCurNode->_f);

	printf("\n=====================================\n");
}


bool JumpPointSearch::CompareG::operator()(Node*& pNode) const
{
	return (pNode->_pos == pos && pNode->_g > g);
}