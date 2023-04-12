#include "PackingMenu.h"

void PrintMenu()
{
	_tprintf(_T(
		"1. Pack\n"
		"2. Unpack\n"
		"3. Show All Files in Packfile\n"
		"4. Add Files to Packing File\n"
		"5. Select Files to Unpack\n"
		"6. Select File to Update\n"
		"7. Clear Console Window\n\n"

		"Choose number and press enter\n")
	);
}

void SelectMenu()
{
	TCHAR input[MENU_INPUT] = { '\0', };
	_fgetts(input, MENU_INPUT, stdin);
	DWORD select = _ttoi(input);

	switch (select)
	{
	case PACK:
		Menu_Pack();
		break;

	case UNPACK:
		Menu_Unpack();
		break;

	case SHOW_FILES:
		Menu_ShowFiles();
		break;

	case ADD_PACK:
		Menu_AddPack();
		break;

	case SELECT_UNPACK:
		Menu_UnpackFiles();
		break;

	case SELECT_UPDATE:
		Menu_UpdateFile();
		break;

	case CLEAR_CONSOLE:
		system("cls");
		break;

	default:
		_tprintf(_T("Wrong input. Choose Again\n"));
	}
}

// Menu

void Menu_Pack()
{
	// Get PackFile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name. (max length %u)\n"), FILE_NAME);
	_fgetts(packName, FILE_NAME, stdin);
 
	// Get Files' Name to Pack
	TCHAR fileNames[INPUT] = { ' ', };
	_tprintf(_T("Enter files' name with extension to pack." 
		" (max count %d, max length %d)\n"), FILE_CNT, INPUT);
	_fgetts(fileNames, INPUT, stdin);

	Pack(packName, fileNames);

}

void Menu_Unpack()
{
	// Get PackFile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name to unpack.\n"));
	_fgetts(packName, FILE_NAME, stdin);

	Unpack(packName);
}

void Menu_ShowFiles()
{
	// Get PackFile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name to add file.\n"));
	_fgetts(packName, FILE_NAME, stdin);

	//ReadPackFile(packName, pack);
	ShowFiles(packName);

	_tprintf(_T("Show All Files Complete!\n\n"));
}

void Menu_AddPack()
{
	// Get PackFile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name to add file.\n"));
	_fgetts(packName, FILE_NAME, stdin);

	// Get Files' Name to Pack
	TCHAR fileNames[INPUT] = { ' ', };
	_tprintf(_T("Enter files name with extension to add."
		" (max count %d, max length %d)\n"), FILE_CNT, INPUT);
	_fgetts(fileNames, INPUT, stdin);

	// Add Files To Pack
	AddPack(packName, fileNames);
}

void Menu_UnpackFiles()
{
	// Get PackFile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name to unpack.\n"));
	_fgetts(packName, FILE_NAME, stdin);

	// Get Files' Name to Pack
	TCHAR fileNames[INPUT] = { ' ', };
	_tprintf(_T("Enter files name with extension to add."
		" (max count %d, max length %d)\n"), FILE_CNT, INPUT);
	_fgetts(fileNames, INPUT, stdin);

	// Write Data to Files
	UnpackFiles(packName, fileNames);
}

void Menu_UpdateFile()
{
	// Get Packfile Name
	TCHAR packName[FILE_NAME] = { '\0', };
	_tprintf(_T("Enter pack file name to unpack.\n"));
	_fgetts(packName, FILE_NAME, stdin);

	// Get File Name to Update
	TCHAR fileName[FILE_NAME] = { ' ', };
	_tprintf(_T("Enter file name with extension to update."
		" (max length %d)\n"), FILE_NAME);
	_fgetts(fileName, FILE_NAME, stdin);

	// Update Data
	UpdateFile(packName, fileName);

}

