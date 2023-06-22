#include "ContentManager.h"
#include "Protocol.h"

ContentManager::ContentManager()
{
}

ContentManager::~ContentManager()
{
}

ContentManager* ContentManager::GetInstance()
{
	static ContentManager _contentMgr;
	return &_contentMgr;
}

void ContentManager::Update()
{
}
