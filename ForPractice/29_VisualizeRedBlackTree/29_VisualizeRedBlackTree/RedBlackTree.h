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
			printf("Success to Insert %d!\n", data);
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
					BalanceCheck(pNewNode);
					printf("Success to Insert %d!\n", data);
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
					BalanceCheck(pNewNode);
					printf("Success to Insert %d!\n", data);
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
		bool bNodeLeft = false;
		if (!SearchNode(data, pNode, bNodeLeft))
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
			else if (bNodeLeft)
			{
				pNode->_pParent->_pLeft = _pNil;
			}
			else if (!bNodeLeft)
			{
				pNode->_pParent->_pRight = _pNil;
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
			else if (bNodeLeft)
			{
				pNode->_pRight->_pParent = pNode->_pParent;
				pNode->_pParent->_pLeft = pNode->_pRight;
			}
			else if (!bNodeLeft)
			{
				pNode->_pRight->_pParent = pNode->_pParent;
				pNode->_pParent->_pRight = pNode->_pRight;
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
			else if (bNodeLeft)
			{ 
				pNode->_pLeft->_pParent = pNode->_pParent;
				pNode->_pParent->_pLeft = pNode->_pLeft;
			}
			else if (!bNodeLeft)
			{ 
				pNode->_pLeft->_pParent = pNode->_pParent;
				pNode->_pParent->_pRight = pNode->_pLeft;
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
		InorderPrintNodeData(_pRoot);
		printf("\n");
	}
	
	void InorderPrintNodeData(Node* pNode)
	{
		if (pNode == _pNil) return;

		InorderPrintNodeData(pNode->_pLeft);

		if(pNode == _pRoot)
			printf("%d: root, left - %d, right - %d\n", 
				pNode->_data, pNode->_pLeft->_data, pNode->_pRight->_data);

		else
			printf("%d: parent - %d, left - %d, right - %d\n",
				pNode->_data, pNode->_pParent->_data,
				pNode->_pLeft->_data, pNode->_pRight->_data);

		InorderPrintNodeData(pNode->_pRight);
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
	void BalanceCheck(Node* pNode)
	{
		printf("\n");
		while (pNode->_pParent->_color != BLACK)
		{
			bool bParentLeft = false;
			if (pNode->_pParent == pNode->_pParent->_pParent->_pLeft)
				bParentLeft = true;

			// if Parent is RED, Uncle is RED 
			if (bParentLeft
				&& pNode->_pParent->_pParent->_pRight->_color == RED)
			{
				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_pRight->_color = BLACK;

				if (pNode->_pParent->_pParent == _pRoot)
				{
					printf("grand parent is root\n");
					break;
				}
				else
				{
					printf("parent left, uncle red\n");
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent;
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
					printf("grand parent is root\n");
					break;
				}
				else
				{
					printf("parent right, uncle red\n");
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent;
					continue;
				}
			}

			else
			{
				// if Parent is RED, Uncle is BLACK 
				bool bRight = false;
				if (pNode == pNode->_pParent->_pRight)
					bRight = true;

				Node* pParent = pNode->_pParent;
				Node* pGrandParent = pNode->_pParent->_pParent;

				if (bRight)
				{
					RotateLeft(pNode->_pParent);
				}
				RotateRight(pGrandParent);
				pParent->_color = BLACK;
				pGrandParent->_color = RED;
				break;
				
			}
		}
		printf("\n");
	}

	void RotateLeft(Node* pNode)
	{
		printf("pNode: %d\n", pNode->_data);
		printf("pNode->_pRight: %d\n", pNode->_pRight->_data);
		if (pNode != _pRoot)
			printf("pNode->_pParent: %d\n", pNode->_pParent->_data);
		else
			printf("pNode->_pParent: root\n");

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pRight;
			pNode->_pRight->_color = BLACK;
		}

		Node* pTmp;
		if (pNode->_pRight == _pNil)
		{
			pTmp = _pNil;
		}
		else
		{
			pTmp = pNode->_pRight->_pLeft;
		}
		
		if(pNode->_pRight->_pLeft == _pNil)
		{
			pNode->_pRight->_pLeft = pNode;
		}
		else if (pNode->_pRight->_pLeft->_data < pNode->_data)
		{

		}
		else if (pNode->_pRight->_pLeft->_data > pNode->_data)
		{

		}

		pNode->_pRight->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pRight;
		pNode->_pRight = pTmp;

	}

	void RotateRight(Node* pNode)
	{
		printf("pNode: %d\n", pNode->_data);
		printf("pNode->_pLeft: %d\n", pNode->_pLeft->_data);
		if (pNode != _pRoot)
			printf("pNode->_pParent: %d\n", pNode->_pParent->_data);
		else
			printf("pNode->_pParent: root\n");

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pLeft;
			pNode->_pLeft->_color = BLACK;
		}

		Node* pTmp;
		if (pNode->_pLeft == _pNil)
		{
			pTmp = _pNil;
		}
		else
		{
			pTmp = pNode->_pLeft->_pRight;
		}

		if (pNode->_pRight->_pRight == _pNil)
		{
			pNode->_pLeft->_pRight = pNode;
		}
		else if (pNode->_pLeft->_pRight->_data < pNode->_data)
		{

		}
		else if (pNode->_pLeft->_pRight->_data > pNode->_data)
		{

		}

		pNode->_pLeft->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pLeft;
		pNode->_pLeft = pTmp;
	}

public:

	void DrawAllNode(HDC hdc, HPEN black, HPEN red, int xPad, int yPad)
	{
		HPEN hPen = (HPEN)SelectObject(hdc, black);

		printf("\n");
		DrawNode(hdc, red, _pRoot,
			X_PIVOT, Y_PIVOT, X_PIVOT, Y_PIVOT,
			xPad, yPad, 0, 0, 0);
		printf("\n");
		SelectObject(hdc, hPen);
	}

	void DrawNode(HDC hdc, HPEN red, 
		Node* pNode, int prevXPos, int prevYPos,
		int xPos, int yPos, int xPad, int yPad,
		int left, int right, int depth)
	{
		/*
		if (pNode == _pRoot)
			printf("root\n");
		printf("Data: %d, Depth: %d, Left: %d, Right:%d\n",
			pNode->_data, depth, left, right);
		*/

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

