#include "Node.h"
#include <stdio.h>

void Node::SetData(Pos pos, int g, int h, Node* pParent)
{
	_pos = pos;
	_g = g;
	_h = h;
	_f = g + h;
	_pParent = pParent;
}

void Node::ResetParent(int g, Node* pParent)
{
	_g = g;
	_f = _g + _h;
	_pParent = pParent;
}
