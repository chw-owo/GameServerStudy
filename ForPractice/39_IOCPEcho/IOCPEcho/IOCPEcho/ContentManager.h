#pragma once
class ContentManager
{
private:
	ContentManager();
	~ContentManager() {}

public:
	static ContentManager* GetInstance();

private:
	static ContentManager _contentManager;
};

