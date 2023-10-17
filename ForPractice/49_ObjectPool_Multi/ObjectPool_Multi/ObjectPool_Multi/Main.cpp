#include "CObjectPoolTester.h"

/*

<비교 테스트>
1. new-delete 
2. ObjectPool Basic Lock ver
3. LockFreePool
4. ObjectPool Minimum Lock ver

*/

int main()
{
	CObjectPoolTester* pTester = new CObjectPoolTester;
	pTester->Test();

	return 0;
}
