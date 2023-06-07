#include "Astar.h"

Astar::Astar()
{
}

Astar::~Astar()
{
}

void Astar::SetStartDestPoint(Pos startPos, Pos destPos)
{
	if (_pStart == nullptr)
	{
		printf("create start node (%d, %d)\n", startPos._x, startPos._y);
		_pStart = new Node(startPos, 0,
			abs(destPos._x - startPos._x) + abs(destPos._y - startPos._y));
	}
	else
	{
		printf("reset start node (%d, %d)\n", startPos._x, startPos._y);
		int dist = abs(destPos._x - startPos._x) + abs(destPos._y - startPos._y);
		
		_pStart->_pos = startPos;
		_pStart->_g = 0;
		_pStart->_h = dist;
		_pStart->_f = dist;
		_pStart->_pParent = nullptr;
	}

	_openSet.insert(*_pStart);

	if (_pDest == nullptr)
	{
		printf("create dest node (%d, %d)\n", destPos._x, destPos._y);
		_pDest = new Node(destPos, -1, 0);
	}
	else
	{
		printf("reset dest node (%d, %d)\n", destPos._x, destPos._y);
		_pDest->_pos = destPos;
		_pDest->_g = -1;
		_pDest->_h = 0;
		_pDest->_f = -1;
		_pDest->_pParent = nullptr;
	}
}

void Astar::FindPath()
{
	
	if (_pStart == nullptr || _pDest == nullptr)
	{
		printf("Set Start-Dest Point First\n");
		return;
	}
	
	// 원래 while(curNode._pos != _pDest->_pos)을 썼지만
	// 테스트를 위해 구조를 바꿨음

	if (_pCurNode == nullptr)
	{
		printf("Start Find Path\n");
		_bComplete = false;
		_pCurNode = _pStart;
		forDubug = 0;
	}

	printf("\n==========================================\n%d\n", forDubug);
	forDubug++;

	if(_pCurNode->_pos != _pDest->_pos)
	{
		CreateNode(_pCurNode);

		set<Node>::iterator iter = _openSet.begin();
		if (iter == _openSet.end())
		{
			printf("Can't Find Path!\n");
			return;
		}

		_pCurNode = const_cast<Node*>(&(*iter));
		printf("Change Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
	}
	else
	{
		printf("Complete Find Path\n");
		_bComplete = true;
		_pCurNode = nullptr;
	}

	if(_bComplete)
	{
		printf("Now Drawing Path\n");
		Pos pos;
		pos = _pDest->_pos;
		while (pos != _pStart->_pos)
		{
			_resultPath.push_front(pos);
		}
	}

	printf("\n==========================================\n");
}

void Astar::CreateNode(Node* pCur)
{
	_openSet.erase(*_pCurNode);
	_closeMap.insert(make_pair(_pCurNode->_pos, _pCurNode));

	if (_pCurNode != _pStart && _pCurNode != _pDest)
	{
		printf("Close Node (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
		g_chMap[_pCurNode->_pos._y][_pCurNode->_pos._x] = CLOSE;
	}
	else if (_pCurNode == _pStart)
	{
		printf("Close Node, but pCur == _pStart (%d, %d), (%d, %d)\n", 
			_pCurNode->_pos._x, _pCurNode->_pos._y, _pStart->_pos._x, _pStart->_pos._y);
	}
	else if (_pCurNode == _pDest)
	{
		printf("Close Node, but pCur == _pDest (%d, %d)\n", _pCurNode->_pos._x, _pCurNode->_pos._y);
	}

	printf("\n");

	Node* pNew;
	for(int i = (int)DIR::UP; i < (int)DIR::END; i++)
	{
		Pos newPos = pCur->_pos + _Direction[i];
		if (!CanGo(newPos)) 
		{
			printf("%d. Can't go (%d, %d)\n", i, newPos._x, newPos._y);
			continue;
		}

		map<Pos, Node*>::iterator existed = _closeMap.find(newPos);

		if (existed != _closeMap.end())
		{
			printf("%d. Reset Parent (%d, %d)\n", i, newPos._x, newPos._y);
			(existed->second)->ResetParent(pCur->_g + 1, pCur);
		}
		else
		{
			
			pNew = new Node(
				newPos,
				pCur->_g + 1,
				abs(_pDest->_pos._x - newPos._x)
				+ abs(_pDest->_pos._y - newPos._y),
				pCur
			);

			printf("%d. Create New Node (%d, %d)\n", i, newPos._x, newPos._y);

			printf("\n");

			_openSet.insert(*pNew);

			if (pNew != _pStart && pNew != _pDest)
			{
				printf("New Node (%d, %d)\n", newPos._x, newPos._y);
				g_chMap[pNew->_pos._y][pNew->_pos._x] = OPEN;
			}
			else if (pNew == _pStart)
			{
				printf("New Node, but pNew == _pStart (%d, %d)\n", newPos._x, newPos._y);
			}
			else if (pNew == _pDest)
			{
				printf("New Node, but pNew == _pDest (%d, %d)\n", newPos._x, newPos._y);
			}

		}
	}
}

bool Astar::CanGo(Pos pos)
{
	if (pos._x < 0 || pos._y < 0 ||
		pos._x >= X_MAX || pos._y >= Y_MAX ||
		g_chMap[pos._y][pos._x] == OBSTACLE)
	{
		return false;
	}

	return true;
}

void Astar::Node::ResetParent(int g, Node* pParent)
{
	_g = g;
	_f = _h + _g;
	_pParent = pParent;
}
