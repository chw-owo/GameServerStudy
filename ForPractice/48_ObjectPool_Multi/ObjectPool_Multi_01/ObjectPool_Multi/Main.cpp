#include "CObjectPoolTester.h"

/*
이상한 점이 너무 많아!!!

1. 
일단 멀티스레드에서 new-delete가 너무 빠름
싱글스레드에서의 new-delete와 비교해도 거의 차이가 안남
성환님이 최소 몇십배는 차이 났다고 하시는 거 보면 이상한 게 맞는 듯

2. 
BasicLock에서 Lock을 얻는 부분이 오래 걸리는 건 당연한데
tail을 넣고 빼기만 하는 것도 new-delete보다 오래 걸린다...
지금 multi new-delete 측정에 문제가 있는 것 같음

*/
int main()
{
	CObjectPoolTester* pTester = new CObjectPoolTester;

	pTester->ProfileSingleNewDelete();
	pTester->ProfileNewDelete();
	pTester->ProfileBasicPool();
	// pTester->ProfileLockFreePool();
	// pTester->ProfileMinLockPool();

	// pTester->Test();
	// pTester->TestLoop();
	return 0;
}
