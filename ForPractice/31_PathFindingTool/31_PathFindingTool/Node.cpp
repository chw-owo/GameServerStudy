#include "Node.h"

void Node::SetData(Pos pos, double g, double h, Node* pParent)
{
	_pos = pos;
	_g = g;
	_h = h;
	_f = g + h;
	_pParent = pParent;
}

void Node::ResetParent(double g, Node* pParent)
{
	_g = g;
	_f = _h + _g;
	_pParent = pParent;
}
