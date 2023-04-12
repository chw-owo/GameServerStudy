#include <windows.h>
#include <stdio.h>
#include "Profile.h"
#include "FileIO.h"

void AlphaBlending();
void ReadBmpHeader(FILE** file, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi);
void ReadBmpData(FILE** file, int size, char* output);
void WriteBmpFile(FILE** output, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi, int size, char* data);

int main()
{
	for(int i = 0; i < 50; i++)
		AlphaBlending();
	PRO_OUT(L"output.txt");
	return 0;
}

void AlphaBlending()
{
	int size;
	WCHAR fileName1[] = L"sample.bmp";
	WCHAR fileName2[] = L"sample2.bmp";
	WCHAR resultFileName[] = L"result.bmp";

	FILE* buffer1 = nullptr;
	FILE* buffer2 = nullptr;
	FILE* result = nullptr;
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;

	// 1. Read Bitmap File

	// 1) Get File Header
	PRO_BEGIN(L"Get File");
	OpenFile(fileName1, &buffer1, L"rb");
	ReadBmpHeader(&buffer1, &bf, &bi);
	size = (bi.biWidth * bi.biHeight * bi.biBitCount) / 8;

	char* pImageBuffer1 = static_cast<char*>(malloc(size));
	char* pImageBuffer2 = static_cast<char*>(malloc(size));
	char* pImageBlend = static_cast<char*>(malloc(size));

	// 2) Get File1 Data
	ReadBmpData(&buffer1, size, pImageBuffer1);
	CloseFile(&buffer1);

	// 3) Get File2 Data
	OpenFile(fileName2, &buffer2, L"rb");
	ReadBmpData(&buffer2, size, pImageBuffer2);
	CloseFile(&buffer2);
	PRO_END(L"Get File");

	// 2. Write File data

	// 1) Get Blending Data
	PRO_BEGIN(L"Blend Data");
	DWORD* pDest = (DWORD*)pImageBlend;
	DWORD* pSrc1 = (DWORD*)pImageBuffer1;
	DWORD* pSrc2 = (DWORD*)pImageBuffer2;

	for (int h = 0; h < bi.biHeight; h++)
	{
		for (int w = 0; w < bi.biWidth; w++)
		{
			*pDest = ((*pSrc1 / 2) & 0x7f7f7f7f) + ((*pSrc2 / 2) & 0x7f7f7f7f);
			pDest++;
			pSrc1++;
			pSrc2++;
		}
	}
	PRO_END(L"Blend Data");

	// 2) Write Bmp File
	PRO_BEGIN(L"Write File");
	OpenFile(resultFileName, &result, L"wb");
	WriteBmpFile(&result, &bf, &bi, size, pImageBlend);
	CloseFile(&result);
	PRO_END(L"Write File");

	free(pImageBlend);
	free(pImageBuffer1);
	free(pImageBuffer2);

}

void ReadBmpHeader(FILE** file, BITMAPFILEHEADER* bf, BITMAPINFOHEADER* bi)
{
	PRO_BEGIN(L"Get BMP Header");
	errno_t ret;

	fread(bf, sizeof(*bf), 1, *file);
	ret = ferror(*file);

	if (ret != 0)
		printf("Fail to read file header : %d\n", ret);

	fread(bi, sizeof(*bi), 1, *file);
	ret = ferror(*file);

	if (ret != 0)
		printf("Fail to read info header : %d\n", ret);
	PRO_END(L"Get BMP Header");
}

void ReadBmpData(FILE** file, int size, char* data)
{
	PRO_BEGIN(L"Get BMP Data");
	errno_t ret;
	fread(data, size, 1, *file);
	ret = ferror(*file);

	if (ret != 0)
		printf("Fail to read data : %d\n", ret);

	PRO_END(L"Get BMP Data");
}

void WriteBmpFile(FILE** output, BITMAPFILEHEADER* bf,
	BITMAPINFOHEADER* bi, int size, char* data)
{
	errno_t ret;

	fwrite(bf, sizeof(*bf), 1, *output);
	ret = ferror(*output);

	if (ret != 0)
		printf("Fail to write file header : %d\n", ret);

	fwrite(bi, sizeof(*bi), 1, *output);
	ret = ferror(*output);

	if (ret != 0)
		printf("Fail to write info header : %d\n", ret);

	fwrite(data, size, 1, *output);
	ret = ferror(*output);

	if (ret != 0)
		printf("Fail to write data : %d\n", ret);
}
