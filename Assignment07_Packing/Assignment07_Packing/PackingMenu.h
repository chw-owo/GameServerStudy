#pragma once
#include "Packing.h"

enum MENU
{
	PACK = 1,
	UNPACK,
	SHOW_FILES,
	ADD_PACK,
	SELECT_UNPACK,
	SELECT_UPDATE,
	CLEAR_CONSOLE
};

//Menu
void SelectMenu();
void PrintMenu();

void Menu_Pack();
void Menu_Unpack();
void Menu_ShowFiles();
void Menu_AddPack();
void Menu_UnpackFiles();
void Menu_UpdateFile();

