#include "AStar.h"
#include "Node.h"
#include <stdio.h>
#include <set>
#include <map>
using namespace std;

void AStar::FindPath()
{
	if (!_bOn) return;

	if(_pCurNode == nullptr)
	{
		_pCurNode = _pNodeMgr->_openSet.top();
		_pNodeMgr->_openSet.pop();
	}

	if (_pCurNode->_pos != _pNodeMgr->_pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
		CreateNode(_pCurNode);

		if (_pNodeMgr->_openSet.empty())
		{
			printf("Can't Find Path!\n");
			return;
		}

		_pNodeMgr->_closeSet.push(_pCurNode);
		_pNodeMgr->_pMap->SetMapState(_pCurNode->_pos._x, _pCurNode->_pos._y, Map::CLOSE);
		_pCurNode = _pNodeMgr->_openSet.top();
		_pNodeMgr->_openSet.pop();
	}
	else
	{
		printf("Complete Find Path\n");
		_bOn = false;
		_pNodeMgr->_pDest = _pCurNode;
		_pCurNode = nullptr;
		// FindShortcut();
	}
}

void AStar::CreateNode(Node* pCurNode)
{

	// 수직 수평 노드 생성
	for (int i = (int)DIR::UP; i < (int)DIR::UP_R; i++)
	{
		Pos newPos = pCurNode->_pos + _direction[i];
		Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);

		switch (state)
		{
		case Map::EMPTY:
		case Map::START:
		case Map::DEST:
		{
			Node* pNew = new Node(
				newPos,
				pCurNode->_g + 1,
				newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
				pCurNode
			);

			printf("%d. Create New Node (%d, %d) (parent: %d, %d)\n",
				i, pNew->_pos._x, pNew->_pos._y,
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y);

			_pNodeMgr->_openSet.push(pNew);
			_pNodeMgr->_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
		}
			break;

		case Map::OBSTACLE:
		case Map::RANGE_OUT:
			printf("%d. Can't Go (%d, %d)\n", i, newPos._x, newPos._y);

		case Map::OPEN:
		case Map::CLOSE:
			printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			break;
		}
	}

	// 대각선 노드 생성
	for (int i = (int)DIR::UP_R; i < (int)DIR::END; i++)
	{
		Pos newPos = pCurNode->_pos + _direction[i];
		Map::STATE state = _pNodeMgr->_pMap->GetMapState(newPos._x, newPos._y);
		switch (state)
		{
		case Map::EMPTY:
		case Map::START:
		case Map::DEST:
		{
			Node* pNew = new Node(
				newPos,
				pCurNode->_g + sqrt(2),
				newPos.GetDistance(_pNodeMgr->_pMap->_destPos),
				pCurNode
			);

			printf("%d. Create New Node (%d, %d) (parent: %d, %d)\n",
				i, pNew->_pos._x, pNew->_pos._y,
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y);

			_pNodeMgr->_openSet.push(pNew);
			_pNodeMgr->_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
		}
			break;

		case Map::OBSTACLE:
		case Map::RANGE_OUT:
			printf("%d. Can't Go (%d, %d)\n", i, newPos._x, newPos._y);
			break;

		case Map::OPEN:
		case Map::CLOSE:
			printf("%d. Already Exist (%d, %d)\n", i, newPos._x, newPos._y);
			break;
		}
	}
}

void AStar::FindShortcut()
{
	Node* pNode = _pNodeMgr->_pDest;
	while (pNode != _pNodeMgr->_pStart)
	{
		
	}
}



