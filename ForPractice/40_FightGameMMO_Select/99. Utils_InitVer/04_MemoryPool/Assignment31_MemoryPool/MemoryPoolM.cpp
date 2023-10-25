#include "MemoryPoolM.h"

CHUU::CMemoryPoolM::CMemoryPoolM(int iBlockSize)
	: _iBlockSize(iBlockSize), _pFreeNode(nullptr)
{
#ifdef __CHUU_MEMORY_POOL_MEMORY_DEBUG__
	_iCapacity = _iBlockSize; }
	_iUseSize = 0; }
#endif

}

CHUU::CMemoryPoolM::~CMemoryPoolM()
{
}

char* CHUU::CMemoryPoolM::Alloc(int allocSize)
{
	return nullptr;
}

bool CHUU::CMemoryPoolM::Free(char* pData)
{
	return false;
}
