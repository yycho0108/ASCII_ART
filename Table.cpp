#include "Header.h"
#include <CommCtrl.h>
#pragma comment (lib, "ComCtl32.lib")

std::multimap<unsigned int, TextPic> DensityMap;
unsigned int FontHeight;
LOGFONT LF;

void WriteTable(HDC hdc)
{
	//GET FILE NAME
	TCHAR FPath[MAX_PATH] = L"";
	lstrcpy(FPath, LF.lfFaceName);
	lstrcat(FPath, L".bin");
	if (GetFileAttributes(FPath) != INVALID_FILE_ATTRIBUTES)
		if (MessageBox(hMainWnd, L"This File Already Exists. OverWrite?", FPath, MB_YESNO) == IDNO) return;
	//


	FILE* TableFile = _wfopen(FPath, L"wb");

	//Initialize Font & Range
	SelectObject(hdc, GetStockObject(WHITE_PEN));
	SelectObject(hdc, GetStockObject(WHITE_BRUSH));
	HFONT OldFont = (HFONT)SelectObject(hdc, hFont);
	unsigned int index = IndexStart;
	//

	// Report Progress
	TCHAR CurrentNumberReport[256];
	InitCommonControls();
	HWND hProg = CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER|PBS_SMOOTH, 0, FontHeight + 100, 400, 50, hMainWnd, 0, g_hInst, 0);
	SendMessage(hProg, PBM_SETRANGE32, IndexStart, IndexEnd);
	SendMessage(hProg, PBM_SETPOS, 0, 0); //wParam = Pos
	//
	fwrite(&LF, sizeof(LOGFONT), 1, TableFile);
	while (index<IndexEnd)
	{
		int CM = CheckForMessage();
		if (CM == -1) { DestroyWindow(hProg); return; }
		else if (CM == 1) continue;

		wchar_t c[2] = { static_cast<wchar_t>(index), 0 };
		wsprintf(CurrentNumberReport, L"%u/%u : %u%% Complete", index, IndexEnd, index*100/IndexEnd);
		SetWindowText(hMainWnd, CurrentNumberReport);
		SendMessage(hProg, PBM_SETPOS, (int)index, 0);

		SIZE SZ;
		GetTextExtentExPointW(hdc, c, 1, 50, NULL, NULL, &SZ);
		TextOut(hdc, 0, 0, c, 1);

		unsigned int CX=0, CY=0;
		unsigned int NP = 0;
		unsigned long int Rtot = 0; //BLACK/WHITE ANYWAYS.
		unsigned char LocVal;

		for (int i = 0; i < SZ.cy; i++)
		{
			for (int j = 0; j < SZ.cx; j++)
			{
				LocVal = GetRValue(GetPixel(hdc, j, i));
				Rtot += LocVal;
				if (!LocVal) CY += i, CX += j, NP++; //LocVal == 0(BLACK)
			}
		}


		if (Rtot)
		{
			if (NP) CY /= NP, CX /= NP;
			Rtot /= SZ.cy*SZ.cx;
			std::pair<unsigned int, TextPic> TableBuf = { Rtot, TextPic(index, SZ.cx, SZ.cy,CX,CY) };
			if (fwrite(&TableBuf, 1, sizeof(std::pair<unsigned int, TextPic>), TableFile) != sizeof(std::pair<unsigned int, TextPic>))
				if (MessageBox(hMainWnd, L"ERROR", L"WRITE FAILED", MB_OKCANCEL) == IDCANCEL) break;
		}

		Rectangle(hdc, 0, 0, SZ.cx, SZ.cy);
		++index;

	}

	fflush(TableFile);
	fclose(TableFile);

	RunState.Running = false;

	MessageBeep(MB_OK);
	MessageBox(hMainWnd, FPath, L"COMPLETE", MB_OK);
	SetWindowText(hMainWnd, Title);
	DestroyWindow(hProg);
}

bool LoadTable()
{
	TCHAR lpstrFile[MAX_PATH] = L"";
	FILE* TableFile;

	if (FileName(hMainWnd, lpstrFile, TEXT("TEXT FILES ONLY\0*.BIN;*.TXT\0")))
	{
		TableFile = _wfopen(lpstrFile, L"rb");
	}//TO BE CHANGED.
	else TableFile = fopen("DefaultTextTable.bin", "rb");

	if (!TableFile) return false;

	std::pair<unsigned int, TextPic> TableBuf;
	int n;

	fread(&LF, sizeof(LOGFONT), 1, TableFile); //FONT SELECTION...ish.

	hFont = CreateFontIndirect(&LF);
	FontHeight = abs(LF.lfHeight);

	while (n = fread(&TableBuf, 1, sizeof(std::pair<unsigned int, TextPic>), TableFile))
	{
		DensityMap.insert(TableBuf);
	}
	fclose(TableFile);
	return true;
}

TCHAR* FileName(HWND hWnd, TCHAR* lpstrFile, LPCWSTR lpstrFilter)
{
	TCHAR szFileTitle[MAX_PATH];
	TCHAR InitDir[MAX_PATH];

	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hWnd;
	OFN.lpstrFilter = lpstrFilter;
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = MAX_PATH;
	OFN.lpstrTitle = TEXT("PLEASE SELECT FILE");
	OFN.lpstrFileTitle = szFileTitle;
	OFN.nMaxFileTitle = MAX_PATH;
	GetWindowsDirectory(InitDir, MAX_PATH);
	OFN.lpstrInitialDir = InitDir;
	if (GetOpenFileName(&OFN))
		return lpstrFile;
	else return 0;
}

TCHAR* SaveBmp(HWND hWnd, TCHAR* lpstrFile)
{
	TCHAR szFileTitle[MAX_PATH];
	TCHAR InitDir[MAX_PATH];

	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hWnd;
	OFN.lpstrFilter = L"BMP FILE(*.BMP*)\0*.BMP*\0";
	OFN.lpstrDefExt = L"bmp";
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = MAX_PATH;
	OFN.lpstrTitle = TEXT("Indicate File Name");
	OFN.lpstrFileTitle = szFileTitle;
	OFN.nMaxFileTitle = MAX_PATH;
	GetWindowsDirectory(InitDir, MAX_PATH);
	OFN.lpstrInitialDir = InitDir;
	if (GetSaveFileName(&OFN))
		return lpstrFile;
	else return 0;
}

TCHAR* SaveTxt(HWND hWnd, TCHAR* lpstrFile)
{
	TCHAR szFileTitle[MAX_PATH];
	TCHAR InitDir[MAX_PATH];

	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hWnd;
	OFN.lpstrFilter = L"TXT FILE(*.TXT*)\0*.TXT*\0";
	OFN.lpstrDefExt = L"txt";
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = MAX_PATH;
	OFN.lpstrTitle = TEXT("Indicate File Name");
	OFN.lpstrFileTitle = szFileTitle;
	OFN.nMaxFileTitle = MAX_PATH;
	GetWindowsDirectory(InitDir, MAX_PATH);
	OFN.lpstrInitialDir = InitDir;
	if (GetSaveFileName(&OFN))
		return lpstrFile;
	else return 0;
}