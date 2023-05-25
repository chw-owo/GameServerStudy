#pragma once
#include <stdio.h>

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

	void InsertNode(DATA data)
	{
		if (_pRoot == nullptr)
		{
			_pRoot = new Node(data);
			printf("Success to Insert %d!\n", data);
			return;
		}

		Node* pNode = _pRoot;

		for(;;)
		{
			if (pNode->_data == data)
			{
				printf("중복되는 값을 넣을 수 없습니다: %d\n", data);
				return;
			}

			if (pNode->_data > data)
			{
				if(pNode->_pLeft == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pLeft = pNewNode;
					printf("Success to Insert %d!\n", data);
					return;
				}
				else if (pNode->_pLeft->_data >= data)
				{
					pNode = pNode->_pLeft;
					continue;
				}
				else
				{
					Node* pNewNode = new Node(data);
					Node* pPrevLeft = pNode->_pLeft;
					pNode->_pLeft = pNewNode;
					pNewNode->_pLeft = pPrevLeft;
					printf("Success to Insert %d!\n", data);
					return;
				}
				
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pRight = pNewNode;
					printf("Success to Insert %d!\n", data);
					return;
				}
				else if (pNode->_pRight->_data <= data)
				{
					pNode = pNode->_pRight;
					continue;
				}
				else
				{
					Node* pNewNode = new Node(data);
					Node* pPrevRight = pNode->_pRight;
					pNode->_pRight = pNewNode;
					pNewNode->_pRight = pPrevRight;
					printf("Success to Insert %d!\n", data);
					return;
				}
			}
		}
	}
	
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
					else if(pNode->_pLeft->_data == data)
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
		
		if (pNode->_pLeft == nullptr &&
			pNode->_pRight == nullptr)
		{
			if (pParent == nullptr)
			{
				delete(pNode);
				_pRoot = nullptr;
				printf("0 Success to Delete %d!\n", data);
				return;
			}

			if (bLeft)
				pParent->_pLeft = nullptr;
			else
				pParent->_pRight = nullptr;
			delete (pNode);

			printf("1 Success to Delete %d!\n", data);
			return;
		}
		else if (pNode->_pLeft == nullptr)
		{
			pNode->_data = pNode->_pRight->_data;
			Node* pPrevRight = pNode->_pRight;
			pNode->_pRight = pNode->_pRight->_pRight;
			delete (pPrevRight);
			printf("2 Success to Delete %d!\n", data);
			return;
		}
		else if (pNode->_pRight == nullptr)
		{
			pNode->_data = pNode->_pLeft->_data;
			Node* pPrevLeft = pNode->_pLeft;
			pNode->_pLeft = pNode->_pLeft->_pLeft;
			delete (pPrevLeft);
			printf("3 Success to Delete %d!\n", data);
			return;
		}

		Node* pChild = pNode->_pLeft;
		if (pChild->_pRight == nullptr)
		{
			pNode->_data = pChild->_data;
			pNode->_pLeft = pChild->_pLeft;
			delete (pChild);
			printf("4 Success to Delete %d!\n", data);
			return;
		}

		while(pChild->_pRight != nullptr)
		{
			pChild = pChild->_pRight;
		}
		pNode->_data = pChild->_data;
		pParent->_pRight = nullptr;
		delete (pChild);
		printf("5 Success to Delete %d!\n", data);

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

private:

	void InorderPrintNode(Node* pNode)
	{
		if (pNode == nullptr) return;

		InorderPrintNode(pNode->_pLeft);
		printf("%d", pNode->_data);
		InorderPrintNode(pNode->_pRight);
	}

private:
	Node* _pRoot = nullptr;
	
};



