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
class BasicBinaryTree
{
private:
	class Node
	{
		friend BasicBinaryTree;

	private:
		Node(DATA data) : _data(data) {}
		~Node() {}

	private:
		DATA _data;
		Node* _pLeft = nullptr;
		Node* _pRight = nullptr;
	};

public:
	BasicBinaryTree() {}
	~BasicBinaryTree() {}

	bool InsertNode(DATA data)
	{
		if(_size >= TREE_MAX)
		{
			printf("Tree is full. MAX: %d\n", _size);
			return false;
		}

		if (_pRoot == nullptr)
		{
			_pRoot = new Node(data);
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
				if (pNode->_pLeft == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pLeft = pNewNode;
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
				if (pNode->_pRight == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pRight = pNewNode;
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

	// TO-DO: 오작동~~ 수정해야됨~~
	void DeleteNode(DATA data)
	{
		if (_pRoot == nullptr)
		{
			printf("현재 비어있는 트리입니다.\n");
			return;
		}

		bool bLeft = false;
		Node* pParent = nullptr;
		Node* pNode = _pRoot;

		if (pNode->_data != data)
		{
			for (;;)
			{
				if (pNode->_data > data)
				{
					if (pNode->_pLeft == nullptr)
					{
						printf("존재하지 않는 값입니다: %d\n", data);
						return;
					}
					else if (pNode->_pLeft->_data == data)
					{
						pParent = pNode;
						pNode = pNode->_pLeft;
						bLeft = true;
						break;
					}
					else
					{
						pNode = pNode->_pLeft;
						continue;
					}
				}

				if (pNode->_data < data)
				{
					if (pNode->_pRight == nullptr)
					{
						printf("존재하지 않는 값입니다: %d\n", data);
						return;
					}
					else if (pNode->_pRight->_data == data)
					{
						pParent = pNode;
						pNode = pNode->_pRight;
						break;
					}
					else
					{
						pNode = pNode->_pRight;
						continue;
					}
				}
			}
		}

		if (pParent == nullptr && pNode != _pRoot)
		{
				printf("Tree is broken~\n");
				return;
		}

		if (pNode->_pLeft == nullptr &&
			pNode->_pRight == nullptr)
		{
			if (pNode == _pRoot)
			{
				printf("(0) ");
				delete(pNode);
				_pRoot = nullptr;
				printf("Success to Delete %d!\n", data);
				_size--;
				return;
			}

			printf("(1) ");

			if (bLeft)
				pParent->_pLeft = nullptr;
			else
				pParent->_pRight = nullptr;
			delete (pNode);

			printf("Success to Delete %d!\n", data);
			_size--;
			return;
		}
		else if (pNode->_pLeft == nullptr)
		{
			printf("(2) ");

			pNode->_data = pNode->_pRight->_data;
			Node* pPrevRight = pNode->_pRight;
			pNode->_pRight = pNode->_pRight->_pRight;
			delete (pPrevRight);
			printf("Success to Delete %d!\n", data);
			_size--;
			return;
		}
		else if (pNode->_pRight == nullptr)
		{
			printf("(3) ");

			pNode->_data = pNode->_pLeft->_data;
			Node* pPrevLeft = pNode->_pLeft;
			pNode->_pLeft = pNode->_pLeft->_pLeft;
			delete (pPrevLeft);
			printf("Success to Delete %d!\n", data);
			_size--;
			return;
		}

		Node* pChild = pNode->_pLeft;
		if (pChild->_pRight == nullptr)
		{
			printf("(4) ");

			pNode->_data = pChild->_data;
			pNode->_pLeft = pChild->_pLeft;
			delete (pChild);
			printf("Success to Delete %d!\n", data);
			_size--;
			return;
		}

		printf("(5) ");

		
		while (pChild->_pRight != nullptr)
		{
			pParent = pChild;
			pChild = pChild->_pRight;
		}

		pNode->_data = pChild->_data;
		pParent->_pRight = nullptr;
		delete (pChild);
		printf("Success to Delete %d!\n", data);
		_size--;
		return;
	}

	DATA* SearchNode(DATA data)
	{
		if (_pRoot == nullptr)
		{
			printf("현재 비어있는 트리입니다.\n");
			return nullptr;
		}

		Node* pNode = _pRoot;

		for (;;)
		{
			if (pNode->_data == data)
			{
				printf("Success to Search %d!\n", data);
				return &(pNode->_data);
			}

			if (pNode->_data > data)
			{
				if (pNode->_pLeft == nullptr)
				{
					printf("존재하지 않는 값입니다: %d\n", data);
					return nullptr;
				}
				else
				{
					pNode = pNode->_pLeft;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == nullptr)
				{
					printf("존재하지 않는 값입니다: %d\n", data);
					return nullptr;
				}
				else
				{
					pNode = pNode->_pRight;
					continue;
				}
			}
		}

		printf("존재하지 않는 값입니다: %d\n", data);
		return nullptr;
	}

	void PrintAllNode()
	{
		InorderPrintNode(_pRoot);
		printf("\n");
	}

	void InorderPrintNode(Node* pNode)
	{
		if (pNode == nullptr) return;

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
		if (pNode == nullptr) return;

		InorderGetNode(pNode->_pLeft, dataArray, index);
		dataArray[*index] = pNode->_data;
		(*index)++;
		InorderGetNode(pNode->_pRight, dataArray, index);
	}

public:

	void DrawAllNode(HDC hdc, int xPad, int yPad)
	{
		DrawNode(hdc, _pRoot, 
			X_PIVOT, Y_PIVOT, X_PIVOT, Y_PIVOT, 
			xPad, yPad, 0, 0, 0);
	}

	void DrawNode(HDC hdc, Node* pNode, int prevXPos, int prevYPos,
		int xPos, int yPos, int xPad, int yPad,
		int left, int right, int depth)
	{
		if (pNode == nullptr) return;

		MoveToEx(hdc, prevXPos + xPad, prevYPos + yPad, NULL);
		LineTo(hdc, xPos + xPad, yPos + yPad);
			
		WCHAR text[TEXT_LEN] = { 0 };
		_itow_s(pNode->_data, text, TEXT_LEN, 10);

		Ellipse(hdc, 
			xPos - RADIUS + xPad, 
			yPos - RADIUS + yPad,
			xPos + RADIUS + xPad, 
			yPos + RADIUS + yPad);

		TextOutW(hdc, 
			xPos - TEXT_PAD + xPad, 
			yPos - TEXT_PAD + yPad,
			text, wcslen(text));

		depth++;

		int prevXGap = abs(prevXPos - xPos);
		
		if (pNode == _pRoot)
			prevXGap = xPos;

		left++;
		DrawNode(hdc, pNode->_pLeft, xPos, yPos, 
			xPos - (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		left--;

		right++;
		DrawNode(hdc, pNode->_pRight, xPos, yPos,
			xPos + (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		right--;

		depth--;
		return;
	}

private:
	Node* _pRoot = nullptr;
	int _size = 0;
};

