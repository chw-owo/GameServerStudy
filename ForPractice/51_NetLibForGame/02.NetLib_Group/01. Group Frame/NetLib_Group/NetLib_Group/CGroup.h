#pragma once
#include <vector>
using namespace std;

class CSession;
class CNetServer;
class CGroup
{
	friend CNetServer;

public:
	void Setting(CNetServer* pNet) { _pNet = pNet; }
	void Terminate() { _alive = false; }

protected: 
	virtual void Update() = 0;
	virtual void OnUpdate() = 0;

private:
	bool _alive;
	CNetServer* _pNet;
	vector<CSession*> _sessions;
};

