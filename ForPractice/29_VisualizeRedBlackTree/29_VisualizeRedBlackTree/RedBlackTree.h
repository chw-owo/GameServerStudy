#pragma once
#include <stdio.h>
#include <windows.h>
#include <cmath>

#define TREE_MAX (INT_MAX / 2)
#define TEXT_LEN 8
#define TEXT_PAD 8
#define RADIUS 12

#define X_MAX 1024
#define X_PIVOT (X_MAX / 2)
#define Y_PIVOT 50
#define Y_GAP 50

template<typename DATA>
class RedBlackTree
{
	enum COLOR
	{
		BLACK = 0,
		RED
	};

private:
	class Node
	{
		friend RedBlackTree;

	private:
		Node(DATA data, Node* pChild) 
			: _data(data), _color(RED), _pParent(nullptr), 
			_pLeft(pChild), _pRight(pChild){}

		~Node() {}

	private:
		DATA _data;

		COLOR _color;
		Node* _pParent;
		Node* _pLeft;
		Node* _pRight;
	};

public:
	RedBlackTree() 
	{
		_pNil = new Node(0, nullptr);
		_pNil->_color = BLACK;
		_pRoot = _pNil;
	}
	~RedBlackTree() {}

	bool InsertNode(DATA data)
	{
		if (_size >= TREE_MAX)
		{
			printf("Tree is full. MAX: %d\n", _size);
			return false;
		}

		if (_pRoot == _pNil)
		{
			_pRoot = new Node(data, _pNil);
			_pRoot->_color = BLACK;
			//printf("Success to Insert %d!\n", data);
			_size++;
			return true;
		}

		Node* pNode = _pRoot;

		for (;;)
		{
			if (pNode->_data == data)
			{
				printf("중복되는 값을 넣을 수 없습니다: %d\n", data);
				return false;
			}

			if (pNode->_data > data)
			{
				if (pNode->_pLeft == _pNil)
				{
					Node* pNewNode = new Node(data, _pNil);
					pNode->_pLeft = pNewNode;
					pNewNode->_pParent = pNode;
					InsertBalanceCheck(pNewNode);
					//printf("Success to Insert %d!\n", data);
					_size++;
					return true;
				}
				else
				{
					pNode = pNode->_pLeft;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == _pNil)
				{
					Node* pNewNode = new Node(data, _pNil);
					pNode->_pRight = pNewNode;
					pNewNode->_pParent = pNode;
					InsertBalanceCheck(pNewNode);
					//printf("Success to Insert %d!\n", data);
					_size++;
					return true;
				}
				else
				{
					pNode = pNode->_pRight;
					continue;
				}
			}
		}
		return false;
	}

	bool SearchNode(DATA data, Node*& pNode, bool& bNodeLeft)
	{
		if (_pRoot == _pNil)
		{
			printf("현재 비어있는 트리입니다.\n");
			pNode = nullptr;
			return false;
		}

		pNode = _pRoot;

		while (pNode->_data != data)
		{
			if (pNode->_data > data)
			{
				if (pNode->_pLeft == _pNil)
				{
					printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					return false;
				}
				else
				{
					pNode = pNode->_pLeft;
					bNodeLeft = true;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == _pNil)
				{
					printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					return false;
				}
				else
				{
					pNode = pNode->_pRight;
					bNodeLeft = false;
					continue;
				}
			}
		}

		printf("Success to Search %d!\n", data);
		return true;
	}

	bool DeleteNode(DATA data)
	{
		Node* pNode = nullptr;
		bool bLeft = false;
		if (!SearchNode(data, pNode, bLeft))
			return false;

		// Delete Input Data Node 
		if (pNode->_pLeft == _pNil &&
			pNode->_pRight == _pNil)
		{
			printf("(0) ");

			if (pNode == _pRoot)
			{
				_pRoot = _pNil;
			}
			else if (bLeft)
			{
				pNode->_pParent->_pLeft = _pNil;
				DeleteBalanceCheck(pNode, true);
			}
			else if (!bLeft)
			{
				pNode->_pParent->_pRight = _pNil;
				DeleteBalanceCheck(pNode, false);
			}

			delete(pNode);
			printf("Success to Delete %d!\n", data);
			_size--;
			return true;
		}
		else if (pNode->_pLeft == _pNil)
		{
			printf("(1) ");
			
			if (pNode == _pRoot)
			{
				_pRoot = pNode->_pRight;
			}
			else if (bLeft)
			{
				pNode->_pRight->_pParent = pNode->_pParent;
				pNode->_pParent->_pLeft = pNode->_pRight;
				DeleteBalanceCheck(pNode, true);
			}
			else if (!bLeft)
			{
				pNode->_pRight->_pParent = pNode->_pParent;
				pNode->_pParent->_pRight = pNode->_pRight;
				DeleteBalanceCheck(pNode, false);
			}

			delete(pNode);
			printf("Success to Delete %d!\n", data);
			_size--;
			return true;
		}
		else if (pNode->_pRight == _pNil)
		{
			printf("(2) ");

			if (pNode == _pRoot)
			{ 
				_pRoot = pNode->_pLeft;
			}
			else if (bLeft)
			{ 
				pNode->_pLeft->_pParent = pNode->_pParent;
				pNode->_pParent->_pLeft = pNode->_pLeft;
				DeleteBalanceCheck(pNode, true);
			}
			else if (!bLeft)
			{ 
				pNode->_pLeft->_pParent = pNode->_pParent;
				pNode->_pParent->_pRight = pNode->_pLeft;
				DeleteBalanceCheck(pNode, false);
			}

			delete(pNode);
			printf("Success to Delete %d!\n", data);
			_size--;
			return true;
		}

		// Delete Alternate Node 

		printf("(3) ");

		Node* pAlt = pNode->_pLeft;
		bool bAltLeft = true;

		while (pAlt->_pRight != _pNil)
		{
			pAlt = pAlt->_pRight;
			bAltLeft = false;
		}

		pNode->_data = pAlt->_data;
		DeleteBalanceCheck(pAlt, bAltLeft);

		if (bAltLeft && pAlt->_pLeft != _pNil)
		{
			pAlt->_pLeft->_pParent = pAlt->_pParent;
			pAlt->_pParent->_pLeft = pAlt->_pLeft;
		}
		else if (bAltLeft && pAlt->_pLeft == _pNil)
		{
			pAlt->_pParent->_pLeft = _pNil;
		}
		else if (!bAltLeft && pAlt->_pLeft != _pNil)
		{
			pAlt->_pLeft->_pParent = pAlt->_pParent;
			pAlt->_pParent->_pRight = pAlt->_pLeft;
		}
		else if (!bAltLeft && pAlt->_pLeft == _pNil)
		{ 
			pAlt->_pParent->_pRight = _pNil;
		}

		delete(pAlt);

		printf("Success to Delete %d!\n", data);
		_size--;
		return true;
	}

	void PrintAllNodeData()
	{
		int leafIdx = 0;
		int black = 0;
		int red = 0;
		
		printf("\n");
		InorderPrintNodeData(_pRoot, leafIdx, black, red);
		g_PrevBlack = 0;
		printf("\n");
	}

	int g_PrevBlack = 0;
	
	void InorderPrintNodeData(Node* pNode, int& leafIdx, int black, int red)
	{
		if (pNode == _pNil)
		{
			black++;
			printf("Leaf %d: black-%d, red-%d\n", leafIdx, black, red);
			g_PrevBlack = black;
			leafIdx++;
			return;
		}

		if (pNode->_color == BLACK) black++;
		else if (pNode->_color == RED) red++;

		InorderPrintNodeData(pNode->_pLeft, leafIdx, black, red);
		InorderPrintNodeData(pNode->_pRight, leafIdx, black, red);
	}

	void PrintAllNode()
	{
		InorderPrintNode(_pRoot);
		printf("\n");
	}

	void InorderPrintNode(Node* pNode)
	{
		if (pNode == _pNil) return;

		InorderPrintNode(pNode->_pLeft);
		printf("%d", pNode->_data);
		InorderPrintNode(pNode->_pRight);
	}

	int GetTreeSize()
	{
		return _size;
	}

	void GetAllNode(DATA* dataArray)
	{
		int index = 0;
		InorderGetNode(_pRoot, dataArray, &index);
	}

	void InorderGetNode(Node* pNode, DATA* dataArray, int* index)
	{
		if (pNode == _pNil) return;

		InorderGetNode(pNode->_pLeft, dataArray, index);
		dataArray[*index] = pNode->_data;
		(*index)++;
		InorderGetNode(pNode->_pRight, dataArray, index);
	}

public:
	void InsertBalanceCheck(Node* pNode)
	{
		while (pNode->_pParent->_color != BLACK)
		{
			bool bParentLeft = false;
			if (pNode->_pParent == pNode->_pParent->_pParent->_pLeft)
				bParentLeft = true;

			if (bParentLeft
				&& pNode->_pParent->_pParent->_pRight->_color == RED)
			{
				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_pRight->_color = BLACK;

				if (pNode->_pParent->_pParent == _pRoot)
				{
					break;
				}
				else
				{
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent->_pParent;
					continue;
				}
			}

			else if (!bParentLeft
				&& pNode->_pParent->_pParent->_pLeft->_color == RED)
			{
				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_pLeft->_color = BLACK;

				if (pNode->_pParent->_pParent == _pRoot)
				{
					break;
				}
				else
				{
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent->_pParent;
					continue;
				}
			}
			else if (bParentLeft
				&& pNode->_pParent->_pParent->_pRight->_color == BLACK)
			{
				if (pNode == pNode->_pParent->_pRight)
				{
					RotateLeft(pNode->_pParent);
					pNode = pNode->_pLeft;
				}

				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_color = RED;
				RotateRight(pNode->_pParent->_pParent);
				break;
			}
			else if (!bParentLeft
				&& pNode->_pParent->_pParent->_pLeft->_color == BLACK)
			{
				if (pNode == pNode->_pParent->_pLeft)
				{
					RotateRight(pNode->_pParent);
					pNode = pNode->_pRight;
				}

				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_color = RED;
				RotateLeft(pNode->_pParent->_pParent);
				break;
			}	
		}
	}

	void DeleteBalanceCheck(Node* pNode, bool bLeft)
	{
		if (pNode->_color == RED) return;

		// To-Do
		 
	}

	void RotateLeft(Node* pNode)
	{
		if (pNode->_pRight == _pNil)
		{
			printf("Left Error: %d\n", pNode->_data);
			return;
		}

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pRight;
			pNode->_pRight->_color = BLACK;
		}
		else
		{
			if (pNode == pNode->_pParent->_pRight)
				pNode->_pParent->_pRight = pNode->_pRight;
			else if (pNode == pNode->_pParent->_pLeft)
				pNode->_pParent->_pLeft = pNode->_pRight;
		}

		Node* pGrandChild = pNode->_pRight->_pLeft;
		pNode->_pRight->_pLeft = pNode;
		pNode->_pRight->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pRight;
		pNode->_pRight = pGrandChild;
		pGrandChild->_pParent = pNode;
	}

	void RotateRight(Node* pNode)
	{
		if (pNode->_pLeft == _pNil)
		{
			printf("Right Error: %d\n", pNode->_data);
			return;
		}

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pLeft;
			pNode->_pLeft->_color = BLACK;
		}
		else
		{
			if (pNode == pNode->_pParent->_pRight)
				pNode->_pParent->_pRight = pNode->_pLeft;
			else if (pNode == pNode->_pParent->_pLeft)
				pNode->_pParent->_pLeft = pNode->_pLeft;
		}

		Node* pGrandChild = pNode->_pLeft->_pRight;
		pNode->_pLeft->_pRight = pNode;
		pNode->_pLeft->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pLeft;
		pNode->_pLeft = pGrandChild;
		pGrandChild->_pParent = pNode;
	}

public:

	void DrawAllNode(HDC hdc, HPEN black, HPEN red, int xPad, int yPad)
	{
		HPEN hPen = (HPEN)SelectObject(hdc, black);

		DrawNode(hdc, red, _pRoot,
			X_PIVOT, Y_PIVOT, X_PIVOT, Y_PIVOT,
			xPad, yPad, 0, 0, 0);
		SelectObject(hdc, hPen);
	}

	void DrawNode(HDC hdc, HPEN red, 
		Node* pNode, int prevXPos, int prevYPos,
		int xPos, int yPos, int xPad, int yPad,
		int left, int right, int depth)
	{
		MoveToEx(hdc, prevXPos + xPad, prevYPos + yPad, NULL);
		LineTo(hdc, xPos + xPad, yPos + yPad);

		WCHAR text[TEXT_LEN];
		if (pNode == _pNil)
		{
			wcscpy_s(text, TEXT_LEN, L"nil");
		}
		else
		{
			_itow_s(pNode->_data, text, TEXT_LEN, 10);
		}

		if (pNode->_color == RED)
		{
			HPEN hPen = (HPEN)SelectObject(hdc, red);

			Ellipse(hdc,
				xPos - RADIUS + xPad,
				yPos - RADIUS + yPad,
				xPos + RADIUS + xPad,
				yPos + RADIUS + yPad);

			SelectObject(hdc, hPen);
		}
		else
		{
			Ellipse(hdc,
				xPos - RADIUS + xPad,
				yPos - RADIUS + yPad,
				xPos + RADIUS + xPad,
				yPos + RADIUS + yPad);
		}

		TextOutW(hdc,
			xPos - TEXT_PAD + xPad,
			yPos - TEXT_PAD + yPad,
			text, wcslen(text));

		if (pNode == _pNil) return;

		
		depth++;

		int prevXGap = abs(prevXPos - xPos);
		if (pNode == _pRoot)
			prevXGap = xPos;

		left++;
		DrawNode(hdc, red, pNode->_pLeft, xPos, yPos,
			xPos - (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		left--;

		right++;
		DrawNode(hdc, red, pNode->_pRight, xPos, yPos,
			xPos + (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		right--;

		depth--;
		return;
	}

private:
	Node* _pNil;
	Node* _pRoot;
	int _size = 0;
};

