#include "TreeTester.h"
#include <list> 

#define INPUT_LEN 16
#define CHECKPOINT 5000000

TreeTester::TreeTester()
{
    srand(500);
    //printf("This is Red Black Tree Tester!\n");
    PrintMenu();
    _hBlackPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    _hRedPen = CreatePen(PS_SOLID, 1, RGB(200, 0, 0));
} 

TreeTester::~TreeTester()
{
}

void TreeTester::SearchNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("검색할 숫자를 입력하세요.\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int num = atoi(nodeInput);

    bool ret = false;
    if (_bCompareMode)
    {
        PRO_BEGIN(L"RBT SEARCH");
        ret = _RedBlackTree.SearchNode(num);
        PRO_END(L"RBT SEARCH");

        PRO_BEGIN(L"BST SEARCH");
        _BinaryTree.SearchNode(num);
        PRO_END(L"BST SEARCH");
    }
    else
    {
        ret = _RedBlackTree.SearchNode(num);
    }

    if (ret)
    {
        printf("%d를 찾았습니다!\n", num);
    }
    else
    {
        printf("%d가 존재하지 않습니다.\n", num);
    }

    PrintMenu();
}

void TreeTester::PrintPathData()
{
    _RedBlackTree.PrintAllPath();
    PrintMenu();
}

void TreeTester::PrintNodeData()
{
    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);
    printf("\n전체 개수: %d\n", max);
    for (int i = 0; i < max; i++)
        printf("%d ", data[i]);
    printf("\n");

    PrintMenu();
}

void TreeTester::InsertNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("연속된 값들을 삽입할 수 있습니다.\n"
    "한개만 삽입하려면 시작 값과 마지막 값을 동일하게 입력하세요.\n\n"
    "삽입하고 싶은 시작 값을 입력하세요.\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int startNum = atoi(nodeInput);

    memset(nodeInput, '\0', INPUT_LEN);
    printf("삽입하고 싶은 마지막 값을 입력하세요.\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int endNum = atoi(nodeInput);

    if (startNum < 0 || startNum > INT_MAX) startNum = 0;
    if (endNum < 0 || endNum > INT_MAX) endNum = INT_MAX;

    printf("요청된 범위: %d~%d\n", startNum, endNum);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = startNum; i <= endNum; i++)
    {
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(i);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(i);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(i);
        }
        
        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("트리가 가득 찼습니다!\n" 
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n", 
                i, i - startNum, endNum - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(i);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                i, i - startNum, endNum - i + 1);
            break;
        }
    }
    
    printf("중복 값 개수: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */

    PrintMenu();
}

void TreeTester::InsertRandomNodeUnder9999()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("삽입할 노드 개수를 입력하세요 (MAX: %d)\n", 9999);
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > 9999 || node < 0)
        node = 9999;

    printf("요청된 개수: %d\n", node);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < node; i++)
    {
        int data = rand() % 9999;
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(data);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(data);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(data);
        }

        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("트리가 가득 찼습니다!\n"
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                data, i, node - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(data);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                data, i, node - i + 1);
            break;
        }
    }

    printf("중복 값 개수: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */

    PrintMenu();
}

void TreeTester::InsertRandomNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("삽입할 노드 개수를 입력하세요 (MAX: %d)\n", INT_MAX);
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > INT_MAX|| node < 0)
        node = INT_MAX;

    printf("요청된 개수: %d\n", node);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < node; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(rand1);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(rand1);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(rand1);
        }

        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("트리가 가득 찼습니다!\n"
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                rand1, i, node - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(rand1);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "현재 데이터: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                rand1, i, node - i + 1);
            break;
        }
    }

    printf("Duplicate Data Count: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    PrintMenu();
}

void TreeTester::DeleteNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("연속된 값의 노드들을 삭제할 수 있습니다.\n"
        "한개만 삭제하려면 시작 값과 마지막 값을 동일하게 입력하세요.\n\n"
        "삭제하고 싶은 시작 값을 입력하세요.\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int startNum = atoi(nodeInput);

    memset(nodeInput, '\0', INPUT_LEN);
    printf("삭제하고 싶은 마지막 값을 입력하세요.\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int endNum = atoi(nodeInput);

    if (startNum < 0 || startNum > INT_MAX) startNum = 0;
    if (endNum < 0 || endNum > INT_MAX) endNum = INT_MAX;
    
    list<int> cantFindList;
    DELETE_NODE_RETURN ret;
    for(int i = startNum; i <= endNum; i++)
    {
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT DELETE");
            ret = _RedBlackTree.DeleteNode(i);
            PRO_END(L"RBT DELETE");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.DeleteNode(i);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.DeleteNode(i);
        }

        if (ret == DELETE_NODE_RETURN::CANT_FIND)
        {
            cantFindList.push_back(i);
        }
        else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n");
            printf("현재 값: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                i, i - startNum, endNum - i + 1);
            break;
        }
    }

    printf("찾지 못한 값의 개수: %llu\n", cantFindList.size());
    printf("삭제에 성공한 값의 개수: %llu\n", (unsigned __int64)(endNum - startNum + 1) - cantFindList.size());
    /*list<int>::iterator iter = cantFindList.begin();
    for (; iter != cantFindList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    

    PrintMenu();
}

void TreeTester::DeleteRandomNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("삭제할 노드 개수를 입력하세요. (MAX: %d)\n", 
        _RedBlackTree.GetTreeSize());
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > _RedBlackTree.GetTreeSize() || node < 0)
        node = _RedBlackTree.GetTreeSize();

    printf("요청된 개수: %d\n", node);

    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);

    list<int> cantFindList;
    DELETE_NODE_RETURN ret;

    if (node <= RAND_MAX)
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = (rand() % max);
            while (data[rand1] == -1)
            {
                rand1 = (rand() % max);
            }

            if (_bCompareMode)
            {
                PRO_BEGIN(L"RBT DELETE");
                ret = _RedBlackTree.DeleteNode(data[rand1]);
                PRO_END(L"RBT DELETE");

                PRO_BEGIN(L"BST DELETE");
                _BinaryTree.DeleteNode(data[rand1]);
                PRO_END(L"BST DELETE");
            }
            else
            {
                ret = _RedBlackTree.DeleteNode(data[rand1]);
            }

            data[rand1] = -1;

            if (ret == DELETE_NODE_RETURN::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
            {
                printf("Unknown Error!\n"
                    "현재 값: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                    rand1, i, node - i + 1);
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = rand();
            int rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= max;

            while (data[rand1] == -1)
            {
                rand1 = rand();
                rand2 = rand();
                rand1 = rand1 << 7;
                rand1 |= rand2;
                rand1 %= max;
            }

            if (_bCompareMode)
            {
                PRO_BEGIN(L"RBT DELETE");
                ret = _RedBlackTree.DeleteNode(data[rand1]);
                PRO_END(L"RBT DELETE");

                PRO_BEGIN(L"BST DELETE");
                _BinaryTree.DeleteNode(data[rand1]);
                PRO_END(L"BST DELETE");
            }
            else
            {
                ret = _RedBlackTree.DeleteNode(data[rand1]);
            }

            data[rand1] = -1;

            if (ret == DELETE_NODE_RETURN::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
            {
                printf("Unknown Error!\n"
                    "현재 값: %d, 성공한 개수: %d, 실패한 개수: %d\n",
                    rand1, i, node - i + 1);
                break;
            }
        }
    }

    delete[] data;

    /*
    list<int>::iterator iter = cantFindList.begin();
    for (; iter != cantFindList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    PrintMenu();
}

void TreeTester::TestTree()
{
    printf("테스트를 중단하려면 Enter를 누르세요.\n");
    _RedBlackTree.DeleteAllNode();
    set<int> testSet;

    int insertRand;
    int insertDataAcc = 0;
    int insertDataCnt = 0;

    int deleteRand;
    int deleteDataAcc = 0;
    int deleteDataCnt = 0;

    int idx = 0;

    while (1)
    {
        idx++;

        if (GetAsyncKeyState(VK_RETURN))
            break;

        insertRand = rand();
        if (!InsertForTest(insertRand, testSet)) break;
        if (!GetTreeDataForTest(testSet))  break;
        insertDataCnt += insertRand;

        deleteRand = rand();
        if (!DeleteForTest(deleteRand, testSet)) break;
        if (!GetTreeDataForTest(testSet))  break;
        deleteDataCnt += deleteRand;

        if (insertDataCnt > CHECKPOINT)
        {
            insertDataCnt -= CHECKPOINT;
            insertDataAcc++;
            printf("삽입 성공 개수: %d\n", 
                insertDataAcc * CHECKPOINT + insertDataCnt);
        }

        if (deleteDataCnt > CHECKPOINT)
        {
            deleteDataCnt -= CHECKPOINT;
            deleteDataAcc++;
            printf("삭제 성공 개수: %d\n",
                deleteDataAcc * CHECKPOINT + deleteDataCnt);
        }
    }

    printf( "\n<Test Result>\n"
            "Loop Count: %d\n"
            "삽입 성공 개수: %d\n"
            "삭제 성공 개수: %d\n\n",
            idx,
            insertDataAcc * CHECKPOINT + insertDataCnt,
            deleteDataAcc * CHECKPOINT + deleteDataCnt
    );

    PrintMenu();
}

void TreeTester::SetCompareMode()
{
    char input[INPUT_LEN] = { '\0', };
    printf("Compare Mode를 끄려면 0을, 키려면 1을 누르세요\n");
    fgets(input, INPUT_LEN, stdin);
    int num = atoi(input);

    if (_bCompareMode && num == 0)
    {
        _bCompareMode = false;
        _bDrawRedBlackTree = true;
        printf("Now Compare Mode [OFF]\n");
    }
    else if (!_bCompareMode && num == 1)
    {
        _bCompareMode = true;
        _BinaryTree.DeleteAllNode();
        _BinaryTree.CopyRedBlackTree(&_RedBlackTree);
        printf("Now Compare Mode [ON]\n");
    }
    else if (!_bCompareMode && num == 0)
    {
        printf("Already Compare Mode [OFF]\n");
    }
    else if (_bCompareMode && num == 1)
    {
        printf("Already Compare Mode [ON]\n");

    }
    else
    {
        printf("Wrong Input!\n");
    }

    PrintMenu();
}

void TreeTester::PrintCompareResult()
{
    if (!_bCompareMode) return;

    PRO_PRINT_OUT();
    PRO_OUT(L"output.txt");
    PrintMenu();
}

void TreeTester::ShiftTreeDraw()
{
    if (!_bCompareMode) return;

    if (_bDrawRedBlackTree)
    {
        _bDrawRedBlackTree = false;
    }
    else
    {
        _bDrawRedBlackTree = true;
    }
}

void TreeTester::DrawTree(HDC hdc)
{
    if(_bDrawRedBlackTree)
    {
        _RedBlackTree.DrawAllNode(
            hdc, _hBlackPen, _hRedPen, _iXPad, _iYPad);
    }
    else
    {
        _BinaryTree.DrawAllNode(hdc, _iXPad, _iYPad);
    }
}

void TreeTester::PrintMenu()
{
    if(_bCompareMode)
    {
        printf("\n1. 노드 검색하기\n"
            "2. 노드 삽입하기\n"
            "3. 랜덤 노드 삽입하기 (%d 이하의 양수)\n"
            "4. 랜덤 노드 삽입하기 (%d 이하의 양수)\n"
            "5. 노드 삭제하기\n"
            "6. 랜덤 노드 삭제하기\n"
            "7. 전체 노드 출력하기\n"
            "8. 경로 별 색깔 개수 출력하기\n"
            "ESC. Clear Console\n"
            "===================================\n"
            "9. 트리 테스트\n"
            "0. 기본 이진 트리와 성능 비교하기 [now: ON]\n"
            "Q. 기본 이진 트리와의 성능 비교 출력하기\n"
            "W. 트리 그림 전환\n\n"      
            "Choose number\n",
            9999, INT_MAX);
    }
    else
    {
        printf("\n1. 노드 검색하기\n"
            "2. 노드 삽입하기\n"
            "3. 랜덤 노드 삽입하기 (%d 이하의 양수)\n"
            "4. 랜덤 노드 삽입하기 (%d 이하의 양수)\n"
            "5. 노드 삭제하기\n"
            "6. 랜덤 노드 삭제하기\n"
            "7. 전체 노드 출력하기\n"
            "8. 경로 별 색깔 개수 출력하기\n"
            "ESC. Clear Console\n"
            "===================================\n"
            "9. 트리 테스트\n"
            "0. 기본 이진 트리와 성능 비교하기 [now: OFF]\n\n"         
            "Choose number\n",
            9999, INT_MAX);
    }
}

bool TreeTester::GetTreeDataForTest(set<int>& testSet)
{
    if(_RedBlackTree.TestAllNode() != TEST_RETURN::SUCCESS)
    {
        return false;
    }

    int treeSize = _RedBlackTree.GetTreeSize();
    int* data = new int[treeSize];
    _RedBlackTree.GetAllNode(data);

    if (treeSize != testSet.size())
    {
        printf("data count is different! tree: %d, test set: %llu\n",
            treeSize, testSet.size());
    }

    int idx = 0;
    set<int>::iterator iter = testSet.begin();
    for (; iter != testSet.end(); iter++) {
        
        if (data[idx] != *iter)
        {
            printf("data is different! tree: %d, test set: %d\n",
                data[idx], *iter);
            return false;
        }

        idx++;
    }

    return true;
}

bool TreeTester::InsertForTest(int count, set<int> &testSet)
{
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < count; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(rand1);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.InsertNode(rand1);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(rand1);
        }

        if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Insert Unknown Error! Cur Data: %d\n", rand1);
            return false;
        }
        else if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            break;
        }

        testSet.insert(rand1);
    }
    return true;
}

bool TreeTester::DeleteForTest(int count, set<int>& testSet)
{
    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);

    if (count > max) count = max;
    DELETE_NODE_RETURN ret;
    for (int i = 0; i < count; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;
        rand1 %= max;

        while (data[rand1] == -1)
        {
            rand1 = rand();
            rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= max;
        }

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT DELETE");
            ret = _RedBlackTree.DeleteNode(data[rand1]);
            PRO_END(L"RBT DELETE");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.DeleteNode(data[rand1]);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.DeleteNode(data[rand1]);
        }

        int deleted = data[rand1];
        data[rand1] = -1;

        if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Delete Unknown Error!: %d\n", rand1);
            delete[] data;
            return false;
        }

        testSet.erase(deleted);
    }

    delete[] data;
    return true;
}
