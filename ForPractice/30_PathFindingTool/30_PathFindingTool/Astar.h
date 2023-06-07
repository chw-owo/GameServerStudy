#pragma once
#include "PathFindUtils.h"
#include <set>
#include <map>

class Astar : public PathFinder
{
private:
	class Node
	{
		friend Astar;

	public:
		Node(Pos pos, int g, int h, Node* pParent = nullptr)
			: _pos(pos), _g(g), _h(h), _f(g + h), _pParent(pParent) {}
		~Node() {}
	
	public:
		void ResetParent(int g, Node* pParent);

	public:
		bool operator < (const Node& other) const { return _f < other._f; }
		bool operator > (const Node& other) const { return _f > other._f; }

	private:
		Pos _pos;
		int _g;
		int _h;
		int _f;
		Node* _pParent;
	};

private:
	enum class DIR
	{
		UP = 0,
		UP_R,
		R,
		DOWN_R,
		DOWN,
		DOWN_L,
		L,
		UP_L,
		END = 8
	};

	Pos _Direction[(int)DIR::END] =
	{
		Pos {0, -1},
		Pos {1, -1},
		Pos {1, 0},
		Pos {1, 1},
		Pos {0, 1},
		Pos {-1, 1},
		Pos {-1, 0},
		Pos {-1, -1}
	};

public:
	Astar();
	~Astar();

public:
	void SetStartDestPoint(Pos startPos, Pos destPos);
	void FindPath();

private:
	void CreateNode(Node* pCur);
	bool CanGo(Pos pos);

private:
	Node* _pStart = nullptr;
	Node* _pDest = nullptr;
	Node* _pCurNode = nullptr;

	set<Node> _openSet;
	map<Pos, Node*> _closeMap;

private:
	int forDubug = 1;
};