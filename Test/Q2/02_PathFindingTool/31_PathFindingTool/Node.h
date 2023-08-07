#pragma once
#include "Pos.h"

enum class DIR
{
	UP = 0,
	R,
	DOWN,
	L,
	UP_R = 4,
	DOWN_R,
	DOWN_L,
	UP_L,
	NONE = 8,
	ALL
};

class Node
{
public:
	Node(Pos pos, int g, int h, Node* pParent = nullptr)
		: _pos(pos), _g(g), _h(h), _f(g + h), _dir(DIR::NONE), _searchDir(DIR::NONE), _pParent(pParent) {}

	Node(Pos pos, int g, int h, DIR dir, DIR searchDir, Node* pParent = nullptr)
		: _pos(pos), _g(g), _h(h), _f(g + h), _dir(dir), _searchDir(searchDir), _pParent(pParent) {}

	~Node() {}

public:
	void SetData(Pos pos, int g, int h, Node* pParent = nullptr);
	void SetData(Pos pos, int g, int h, DIR dir, DIR searchDir, Node* pParent = nullptr);
	void ResetParent(int g, Node* pParent);
	void ResetParent(int g, DIR dir, DIR searchDir, Node* pParent);

public:
	bool operator==(const Node& other) { return _f == other._f; }
	bool operator!=(const Node& other) { return _f != other._f; }
	bool operator < (const Node& other) const { return _f < other._f; }
	bool operator > (const Node& other) const { return _f > other._f; }
	bool operator <= (const Node& other) const { return _f <= other._f; }
	bool operator >= (const Node& other) const { return _f >= other._f; }

public:
	Pos _pos;
	int _g;
	int _h;
	int _f;
	DIR _dir;
	DIR _searchDir;
	Node* _pParent;
};

