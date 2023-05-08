#pragma once
enum MSG_TYPE
{
	TYPE_ID = 0,
	TYPE_CREATE,
	TYPE_DELETE,
	TYPE_MOVE
};

struct MSG_ID
{
	MSG_TYPE type = TYPE_ID;
	__int32 ID;
	__int32 none1 = 0;
	__int32 none2 = 0;
};

struct MSG_CREATE
{
	MSG_TYPE type = TYPE_CREATE;
	__int32 ID;
	__int32 X;
	__int32 Y;
};

struct MSG_DELETE
{
	MSG_TYPE type = TYPE_DELETE;
	__int32 ID;
	__int32 none1 = 0;
	__int32 none2 = 0;
};

struct MSG_MOVE
{
	MSG_TYPE type = TYPE_MOVE;
	__int32 ID;
	__int32 X;
	__int32 Y;
};
