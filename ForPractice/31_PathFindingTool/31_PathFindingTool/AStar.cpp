#include "AStar.h"

void AStar::FindPath()
{
	if (!_bOn) return;

	if (_pCurNode == nullptr)
		_pCurNode = _pNodeMgr->_pStart;

	if (_pCurNode->_pos != _pMap->_destPos)
	{
		printf("\n");
		printf("Cur Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
		CreateNode(_pCurNode);

		set<Node*>::iterator iter = _pNodeMgr->_openSet.begin();
		if (iter == _pNodeMgr->_openSet.end())
		{
			printf("Can't Find Path!\n");
			return;
		}

		_pCurNode = *iter; 

		printf("Change Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
		printf("\n");

		printf("=======================================\nOpen: ");
		set<Node*>::iterator openIter = _pNodeMgr->_openSet.begin();
		for (; openIter != _pNodeMgr->_openSet.end(); openIter++)
		{
			printf("(%d, %d) ", (*openIter)->_pos._x, (*openIter)->_pos._y);
		}
		printf("\n\nClose: ");
		map<Pos, Node*>::iterator closeIter = _pNodeMgr->_closeMap.begin();
		for (; closeIter != _pNodeMgr->_closeMap.end(); closeIter++)
		{
			printf("(%d, %d) ", closeIter->second->_pos._x, closeIter->second->_pos._y);
		}
		printf("\n=======================================\n");
		printf("\n");
	}
	else
	{
		printf("Complete Find Path\n");
		_bOn = false;
		_pNodeMgr->_pDest = _pCurNode;
		_pCurNode = nullptr;
	}
}

void AStar::CreateNode(Node* pCurNode)
{
	_pNodeMgr->InsertCloseNode(pCurNode);
	_pMap->SetMapState(pCurNode->_pos._x, pCurNode->_pos._y, Map::CLOSE);

	for (int i = (int)DIR::UP; i < (int)DIR::END; i++)
	{
		Pos newPos = pCurNode->_pos + _direction[i];
		if (!_pMap->CanGo(newPos))
		{
			printf("%d. Can't go (%d, %d)\n", i, newPos._x, newPos._y);
			continue;
		}

		bool exist = false;
		set<Node*>::iterator openIter = _pNodeMgr->_openSet.begin();
		for (; openIter != _pNodeMgr->_openSet.end(); openIter++)
		{
			if ((*openIter)->_pos == newPos)
			{
				exist = true;
				if ((*openIter)->_pParent != nullptr)
				{
					printf("%d. Aleady Exist (%d, %d) (parent: %d, %d)\n",
						i, newPos._x, newPos._y,
						(*openIter)->_pParent->_pos._x,
						(*openIter)->_pParent->_pos._y);
				}
				else
				{
					printf("%d. Aleady Exist (%d, %d)\n", i, newPos._x, newPos._y);
				}
				break;
			}
		}

		map<Pos, Node*>::iterator closeIter = _pNodeMgr->_closeMap.find(newPos);

		if(exist)
		{ 
		
		}
		else if (closeIter != _pNodeMgr->_closeMap.end())
		{
			if(closeIter->second->_pParent != nullptr)
			{
				printf("%d. Aleady Exist (%d, %d) (parent: %d, %d)\n",
					i, newPos._x, newPos._y,
					closeIter->second->_pParent->_pos._x,
					closeIter->second->_pParent->_pos._y);
			}
			else
			{
				printf("%d. Aleady Exist (%d, %d)\n", i, newPos._x, newPos._y);
			}

			//(existed->second)->ResetParent(pCurNode->_g + 1, pCurNode);
		}
		else
		{
			Node* pNew = new Node(
				newPos,
				pCurNode->_g + 1,
				abs(_pMap->_destPos._x - newPos._x)
				+ abs(_pMap->_destPos._y - newPos._y),
				pCurNode
			);

			printf("%d. Create New Node (%d, %d) (parent: %d, %d)\n", 
				i, pNew->_pos._x, pNew->_pos._y, 
				pNew->_pParent->_pos._x, pNew->_pParent->_pos._y);

			_pNodeMgr->InsertOpenNode(pNew);
			_pMap->SetMapState(pNew->_pos._x, pNew->_pos._y, Map::OPEN);
		}
	}
}


