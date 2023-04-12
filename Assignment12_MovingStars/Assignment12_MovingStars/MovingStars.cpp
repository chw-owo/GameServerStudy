#include "MovingStars.h"
#include <tchar.h>

TCHAR oneStar[ONE + 1] = _T("*");
TCHAR twoStar[TWO + 1] = _T("**");
TCHAR threeStar[THREE + 1] = _T("***");

void Star::Update()
{ 
	_x += _interval;

	if (_x + _starSize >= MAX)
	{
		_dead = true;
		return;
	}
		
	int i = 0;

	for (; i < _x; i++)
		_line[i] = _T(' ');

	for (int j = 0; j < _starSize; j++)
	{
		_line[i] = _star[j];
		i++;
	}

	for (; i < MAX; i++)
		_line[i] = _T(' ');

	_line[i] = _T('\n');
}

void Star::Render()
{
	if (_dead == true)
		return;

	for (int i = 0; i <= MAX; i++)
		_tprintf(_T("%c"), _line[i]);
}

bool Star::CheckDead()
{
	return _dead;
}

void OneStar::Initialize()
{
	_interval = ONE;
	_starSize = ONE;
	_star = oneStar;
}

void TwoStar::Initialize()
{
	_interval = TWO;
	_starSize = TWO;
	_star = twoStar;
}

void ThreeStar::Initialize()
{
	_interval = THREE;
	_starSize = THREE;
	_star = threeStar;
}