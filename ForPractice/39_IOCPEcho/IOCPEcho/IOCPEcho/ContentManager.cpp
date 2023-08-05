#include "ContentManager.h"

ContentManager::ContentManager()
{
}

ContentManager* ContentManager::GetInstance()
{
	static ContentManager _contentManager;
	return &_contentManager;
}