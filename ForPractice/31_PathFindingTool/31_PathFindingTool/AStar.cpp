#include "AStar.h"
#include "Node.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
using namespace std;

void AStar::FindPath()
{
	if (!_bFindPathOn) return;

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", 
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		CreateNode(_pNodeMgr->_pCurNode);

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
		//PrintOpenListForDebug();
		pop_heap(_pNodeMgr->_openList.begin(), _pNodeMgr->_openList.end());
		_pNodeMgr->_openList.pop_back();
	}
	else
	{
		printf("Complete Find Path\n=================================\n");
		_pNodeMgr->_pDest = _pNodeMgr->_pCurNode;
		_bFindPathOn = false;
	}
}

void AStar::FindPathStepInto()
{
	if (!_bFindPathStepOn)
	{
		_pNodeMgr->SetData();
		_bFindPathStepOn = true;
		_bFindPathOn = false;
	}

	if (_pNodeMgr->_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", 
			_pNodeMgr->_pCurNode->_pos._x, _pNodeMgr->_pCurNode->_pos._y);
		CreateNode(_pNodeMgr->_pCurNode);

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
		//PrintOpenListForDebug();
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

void AStar::CreateNode(Node* pCurNode)
{

	// 수직 수평 노드 생성
	for (int i = (int)DIR::UP; i <= (int)DIR::L; i++)
	{
		Pos newPos = pCurNode->_pos + _direction[i];
		Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);

		switch (state)
		{
		case Map::NONE:
		case Map::START:
		case Map::DEST:
		{
			Node* pNew = new Node(
				newPos,
				pCurNode->_g + VERT_DIST,
				newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
				pCurNode
			);

			printf("%d. Create New Node (%d, %d) (parent: %d, %d) (%d + %d = %d)\n",
				i, pNew->_pos._x, pNew->_pos._y,
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y, 
				pNew->_g, pNew->_h, pNew->_f);

			_pNodeMgr->_openList.push_back(pNew);
			_pNodeMgr->_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
		}
			break;

		case Map::OPEN:
		{
			compareG.pos = newPos;
			compareG.g = pCurNode->_g + VERT_DIST;

			vector<Node*>::iterator it = find_if(_pNodeMgr->_openList.begin(), 
				_pNodeMgr->_openList.end(), compareG);

			if (it != _pNodeMgr->_openList.end())
			{
				(*it)->ResetParent(pCurNode->_g + VERT_DIST, pCurNode);
				printf("%d. Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n", 
					i, newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
			}
			else
			{
				printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			}	
		}
		break;

		case Map::CLOSE:
		{
			compareG.pos = newPos;
			compareG.g = pCurNode->_g + VERT_DIST;

			vector<Node*>::iterator it = find_if(_pNodeMgr->_closeList.begin(),
				_pNodeMgr->_closeList.end(), compareG);

			if (it != _pNodeMgr->_closeList.end())
			{
				(*it)->ResetParent(pCurNode->_g + VERT_DIST, pCurNode);
				printf("%d. Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
					i, newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
			}
			else
			{
				printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			}
		}
			break;

		case Map::OBSTACLE:
		case Map::RANGE_OUT:
			printf("%d. Can't Go (%d, %d)\n", i, newPos._x, newPos._y);
			break;
		}
	}

	// 대각선 노드 생성
	for (int i = (int)DIR::UP_R; i <= (int)DIR::UP_L; i++)
	{
		Pos newPos = pCurNode->_pos + _direction[i];
		Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);
		switch (state)
		{
		case Map::NONE:
		case Map::START:
		case Map::DEST:
		{
			Node* pNew = new Node(
				newPos,
				pCurNode->_g + DIAG_DIST,
				newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
				pCurNode
			);

			printf("%d. Create New Node (%d, %d) (parent: %d, %d) (%d + %d = %d)\n",
				i, pNew->_pos._x, pNew->_pos._y,
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y,
				pNew->_g, pNew->_h, pNew->_f);

			_pNodeMgr->_openList.push_back(pNew);
			_pNodeMgr->_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
		}
			break;

		case Map::OPEN:
		{
			compareG.pos = newPos;
			compareG.g = pCurNode->_g + DIAG_DIST;

			vector<Node*>::iterator it = find_if(_pNodeMgr->_openList.begin(),
				_pNodeMgr->_openList.end(), compareG);

			if (it != _pNodeMgr->_openList.end())
			{
				(*it)->ResetParent(pCurNode->_g + DIAG_DIST, pCurNode);
				printf("%d. Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
					i, newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
			}
			else
			{
				printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			}

		}
		break;

		case Map::CLOSE:
		{
			compareG.pos = newPos;
			compareG.g = pCurNode->_g + DIAG_DIST;

			vector<Node*>::iterator it = find_if(_pNodeMgr->_closeList.begin(),
				_pNodeMgr->_closeList.end(), compareG);

			if (it != _pNodeMgr->_closeList.end())
			{
				(*it)->ResetParent(pCurNode->_g + DIAG_DIST, pCurNode);
				printf("%d. Already Exist, Reset Parent (%d, %d) (parent: %d, %d)\n",
					i, newPos._x, newPos._y, (*it)->_pParent->_pos._x, (*it)->_pParent->_pos._y);
			}
			else
			{
				printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			}
		}
		break;

		case Map::OBSTACLE:
		case Map::RANGE_OUT:
			printf("%d. Can't Go (%d, %d)\n", i, newPos._x, newPos._y);
			break;
		}
	}
}

void AStar::PrintOpenListForDebug()
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

bool AStar::CompareG::operator()(Node*& pNode) const
{
	return (pNode->_pos == pos && pNode->_g > g);
}

