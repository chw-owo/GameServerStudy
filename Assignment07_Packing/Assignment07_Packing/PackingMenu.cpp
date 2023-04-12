#include "PackingMenu.h"
#include <stdio.h>
#include <stdlib.h>

#define MENU_INPUT 4
#define INPUT 512

void PrintMenu()
{
	printf(
		"1. Pack\n"
		"2. Unpack\n"
		"3. Show All Files in Packfile\n"
		"4. Add Files to Packing File\n"
		"5. Select Files to Unpack\n"
		"6. Select File to Update\n"
		"7. Clear Console Window\n\n"

		"Choose number and press enter\n"
	);
}

void SelectMenu()
{
	char input[MENU_INPUT] = { '\0', };
	fgets(input, MENU_INPUT, stdin);
	int select = atoi(input);

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
		printf("Wrong input. Choose Again\n");
	}
}

// Menu

void Menu_Pack()
{
	// Get PackFile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name. (max length %u)\n", FILE_NAME);
	fgets(packName, FILE_NAME, stdin);
 
	// Get Files' Name to Pack
	char fileNames[INPUT] = { ' ', };
	printf("Enter files' name with extension to pack." 
		" (max count %d, max length %d)\n", FILE_CNT, INPUT);
	fgets(fileNames, INPUT, stdin);

	Pack(packName, fileNames);

}

void Menu_Unpack()
{
	// Get PackFile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name to unpack.\n");
	fgets(packName, FILE_NAME, stdin);

	Unpack(packName);
}

void Menu_ShowFiles()
{
	// Get PackFile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name to add file.\n");
	fgets(packName, FILE_NAME, stdin);

	//ReadPackFile(packName, pack);
	ShowFiles(packName);

	printf("Show All Files Complete!\n\n");
}

void Menu_AddPack()
{
	// Get PackFile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name to add file.\n");
	fgets(packName, FILE_NAME, stdin);

	// Get Files' Name to Pack
	char fileNames[INPUT] = { ' ', };
	printf("Enter files name with extension to add."
		" (max count %d, max length %d)\n", FILE_CNT, INPUT);
	fgets(fileNames, INPUT, stdin);

	// Add Files To Pack
	AddPack(packName, fileNames);
}

void Menu_UnpackFiles()
{
	// Get PackFile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name to unpack.\n");
	fgets(packName, FILE_NAME, stdin);

	// Get Files' Name to Pack
	char fileNames[INPUT] = { ' ', };
	printf("Enter files name with extension to add."
		" (max count %d, max length %d)\n", FILE_CNT, INPUT);
	fgets(fileNames, INPUT, stdin);

	// Write Data to Files
	UnpackFiles(packName, fileNames);
}

void Menu_UpdateFile()
{
	// Get Packfile Name
	char packName[FILE_NAME] = { '\0', };
	printf("Enter pack file name to unpack.\n");
	fgets(packName, FILE_NAME, stdin);

	// Get File Name to Update
	char fileName[FILE_NAME] = { ' ', };
	printf("Enter file name with extension to update."
		" (max length %d)\n", FILE_NAME);
	fgets(fileName, FILE_NAME, stdin);

	// Update Data
	UpdateFile(packName, fileName);
}

