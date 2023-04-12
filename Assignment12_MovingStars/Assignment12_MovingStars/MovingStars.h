#pragma once
#include <stdio.h>
#include <stdlib.h>

#define ONE 1
#define TWO 2
#define THREE 3
#define MAX 75

class BaseObject
{
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual bool CheckDead() = 0;

public:
	bool _dead = false;
	int _x = 0;
	int _interval = 0;
	int _starSize = 0;
	char* _star;
	char _line[MAX + 1] = { '\0',};
};

class Star : public BaseObject
{
public:
	void Update();
	void Render();
	bool CheckDead();
};

class OneStar : public Star
{
public:
	void Initialize();
};

class TwoStar : public Star
{
public:
	void Initialize();
};

class ThreeStar : public Star
{
public:
	void Initialize();
};
