#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <tchar.h>
#include <cstdlib>
#include <algorithm>
#include <ShlObj.h>
#include "Header.h"

using namespace std;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LPCTSTR Title = L"ASCII ART GENERATOR";


void OpenBitMap();
void Execute();

void HandleMenu(WPARAM);
void HandleSizeMenu(WPARAM);
void HandleQualityMenu(WPARAM);
void HandleColorMenu(WPARAM);

void HandleScroll(UINT, WPARAM);
void AdjustScrollBarSize();

void SaveToBitmap();
void SaveToText();

HINSTANCE g_hInst;
HBITMAP TargetBitmap;
HWND hMainWnd;
HDC MemDC;

unsigned int ScreenWidth;
unsigned int ScreenHeight;
unsigned int OutWidth;
unsigned int OutHeight;

int WindowOriginW;
int WindowOriginH;

double ZoomValue = 1;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;
	WNDCLASS WndClass;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = Title;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_VREDRAW | CS_HREDRAW;
	RegisterClass(&WndClass);
	
	hMainWnd = CreateWindow(Title, Title, WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	bool Process = true;
	
	static int MX, MY;
	static bool Panning;

	switch (iMsg)
	{
	case WM_CREATE:
		hMainWnd = hWnd;
		hdc = GetDC(hWnd);
		ScreenWidth = GetDeviceCaps(hdc, HORZRES);
		ScreenHeight = GetDeviceCaps(hdc, VERTRES);
		MemDC = CreateCompatibleDC(hdc);
		ReleaseDC(hWnd, hdc);
		RunState.SimpleDepthFilter = 150;
		if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, OptionDlgProc) == IDCANCEL) return -1;
		RegisterHotKey(hWnd, 0, MOD_CONTROL, 'R');
		RegisterHotKey(hWnd, 1, MOD_CONTROL, 'S'); //Image Save
		RegisterHotKey(hWnd, 2, MOD_CONTROL|MOD_ALT, 'S'); //Txt Save
		break;
	case WM_HOTKEY:
		switch (wParam)
		{
		case 0:SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_RUN, 0), 0);
			break;
		case 1:SendMessage(hWnd, WM_COMMAND, ID_FILE_SAVEIMAGE, 0);
			break;
		case 2:SendMessage(hWnd, WM_COMMAND, ID_FILE_SAVETEXT, 0);
			break;
		}
		break;
	case WM_COMMAND:
		HandleMenu(wParam);
		break;
	case WM_LBUTTONDOWN: //Perhaps I should try Panning?
		//
		MX = LOWORD(lParam);
		MY = HIWORD(lParam);
		Panning = true;
		SetCapture(hWnd);
		break;
	case WM_LBUTTONUP:
		Panning = false;
		ReleaseCapture();
		break;
	case WM_RBUTTONDOWN:
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, OptionDlgProc);
		break;
	case WM_MOUSEMOVE:
		if (Panning)
		{
			int CurX = LOWORD(lParam);
			int CurY = HIWORD(lParam);

			SetScrollPos(hMainWnd, SB_VERT, WindowOriginH += (MY-CurY), TRUE);
			SetScrollPos(hMainWnd, SB_HORZ, WindowOriginW += (MX-CurX), TRUE);
			MX = CurX;
			MY = CurY;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	case WM_MOUSEWHEEL: //zoom
		ZoomValue += ((short)HIWORD(wParam)>0) ? 0.1 : -0.1;
		if (ZoomValue < 0.1) ZoomValue = 0.1;
		else if (ZoomValue > 1.9) ZoomValue = 1.9;
		ResetWindowSize = true;
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawASCII(hdc);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		DeleteObject(TargetBitmap);
		DeleteDC(MemDC);
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	case WM_VSCROLL:
	case WM_HSCROLL:
		HandleScroll(iMsg, wParam);
		break;
	case WM_SIZE:
		AdjustScrollBarSize();
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void DrawASCII(HDC hdc)
{
	unsigned int Width = RunState.PxlTxtEnabled ? (::Width*FontHeight/RunState.PxlsPerTxtW): (::Width);
	unsigned int Height = RunState.PxlTxtEnabled ? (::Height*FontHeight/RunState.PxlsPerTxtH): (::Height);

	//int w, h;
	switch (RunState.Size)
	{
	case RunState.ImageSizeOption::FitScreen:
		OutWidth = ScreenWidth, OutHeight = ScreenHeight;
		break;
	case RunState.ImageSizeOption::ImageSize:
		OutWidth = Width, OutHeight = Height;
		break;
	case RunState.ImageSizeOption::Custom:
		OutWidth = CustomWidth, OutHeight = CustomHeight;
		break;
	case RunState.ImageSizeOption::PreserveRatio:
	{
		if (Width <= ScreenWidth && Height <= ScreenHeight)
		{
			OutWidth = Width, OutHeight = Height;
		}
		else
		{
			if (Width > ScreenWidth && Height > ScreenHeight)
			{
				if (Width*ScreenHeight > Height*ScreenWidth) //width, greater ratio
					OutWidth = ScreenWidth, OutHeight = Height * ScreenWidth / Width;
				else
					OutWidth = Width * ScreenHeight / Height, OutHeight = ScreenHeight;
			}
			else if (Width > ScreenWidth)
				OutWidth = ScreenWidth, OutHeight = Height*ScreenWidth / Width;
			else //if (Height > ScreenHeight)
				OutHeight = ScreenHeight, OutWidth = Width*ScreenHeight / Height;

			
		}
		break;
	}
	}

	OutWidth *= ZoomValue;
	OutHeight *= ZoomValue;

	StretchBlt(hdc, -WindowOriginW, -WindowOriginH, OutWidth, OutHeight, MemDC, 0, 0, Width, Height, SRCCOPY);

	if (OutWidth&&OutHeight)
	{
		if (ResetWindowSize)
		{
			RECT R = { 0, 0, OutWidth, OutHeight };
			DWORD Style = GetWindowLong(hMainWnd, GWL_STYLE);
			AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, TRUE);
			if(Style&WS_VSCROLL) R.right += GetSystemMetrics(SM_CXVSCROLL);
			if(Style&WS_HSCROLL) R.bottom += GetSystemMetrics(SM_CYHSCROLL);
			ResetWindowSize = false;
			if (R.right - R.left <ScreenWidth && R.bottom - R.top < ScreenHeight)
				SetWindowPos(hMainWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE);

			AdjustScrollBarSize(); //w & h = true image width
		}
	}
}


void OpenBitMap()
{
	DeleteObject(TargetBitmap);
	TargetBitmap = NULL;
	TCHAR lpstrFile[MAX_PATH] = L"";
	if (FileName(hMainWnd, lpstrFile, TEXT("BMP FILES ONLY(*.BMP*)\0*.BMP*\0")))
	{
		TargetBitmap = (HBITMAP)LoadImage(NULL, lpstrFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	return;
}

void Execute()
{
	if (RunState.Running || RunState.Pause)
	{
		MessageBox(hMainWnd, L"CANNOT EXECUTE NEW PROCESS WHILE RUNNING", L"ALERT", MB_OK);
		return;
	}
	if (MessageBox(hMainWnd, L"Begin Execution", L"Execution", MB_YESNO) == IDNO) return;
	RunState.Running = true;
	RunState.Pause = false;
	HDC hdc = GetDC(hMainWnd);
	switch (RunState.Mode)
	{
	case RunState.ModeOption::Register:
		WriteTable(hdc);
		break;
	case RunState.ModeOption::Render:
		if (TargetBitmap) RenderText(hdc, TargetBitmap);
		else
		{
			MessageBox(hMainWnd, L"No Bitmap is Currently Selected", L"ERROR", MB_OK);
			RunState.Running = false;
		}
		break;
	}
	ReleaseDC(hMainWnd, hdc);
	return;
}

void HandleMenu(WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case ID_LOAD_FONT: LoadTable();
		break;
	case ID_LOAD_BITMAP: OpenBitMap();
		break;
	case ID_FILE_RUN: Execute();
		break;
	case ID_FILE_SAVEIMAGE: SaveToBitmap();
		break;
	case ID_FILE_SAVETEXT: SaveToText();
		break;
	case ID_FILE_DROP:
		RunState.Running = false;
		if (RunState.Pause)
			PostMessage(hMainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_PAUSE, 0), 0);
		break;
	case ID_FILE_PAUSE:
	{
		//Currently Editing Here
		RunState.Pause = !RunState.Pause;
		MENUITEMINFO PauseInfo;
		memset(&PauseInfo, 0, sizeof(MENUITEMINFO));

		PauseInfo.cbSize = sizeof(MENUITEMINFO);
		PauseInfo.fMask = MIIM_STRING;
		PauseInfo.dwTypeData = RunState.Pause ? L"Resume" : L"Pause";
		SetMenuItemInfo(GetSubMenu(GetMenu(hMainWnd), 0), 2, TRUE, &PauseInfo);
		SetWindowText(hMainWnd, RunState.Pause ? L"PAUSED" : Title);
	}
		break;
	case ID_FILE_SETTINGS: DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hMainWnd, OptionDlgProc);
		break;
	case ID_FILE_EXIT:
		PostQuitMessage(0);
		break;
		//SIZE
	case ID_FITSCREEN_RATIO:
	case ID_FITSCREEN_FITSCREEN:
	case ID_SIZE_IMAGESIZE:
	case ID_SIZE_CUSTOMSIZE:
		HandleSizeMenu(wParam);
		break;
		//QUALITY
	case ID_QUALITY_SIMPLE:
	case ID_QUALITY_PRECISE:
	case ID_QUALITY_WEIGHTED:
		HandleQualityMenu(wParam);
		break;
	case ID_QUALITY_PXLS:
	{
		RunState.PxlTxtEnabled = !RunState.PxlTxtEnabled;
		MENUITEMINFO PxlsInfo;
		memset(&PxlsInfo, 0, sizeof(MENUITEMINFO));

		if (RunState.PxlTxtEnabled && DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG3), hMainWnd, GetDimensionsProc,ID_QUALITY_PXLS) == IDOK)
		{
			TCHAR strbuf[50];
			PxlsInfo.cbSize = sizeof(MENUITEMINFO);
			PxlsInfo.fMask = MIIM_STRING;
			wsprintf(strbuf, L"Pxl/Txt[%uX%u]", RunState.PxlsPerTxtW, RunState.PxlsPerTxtH);
			PxlsInfo.dwTypeData = strbuf;
			SetMenuItemInfo(QualityMenu, 4, TRUE, &PxlsInfo);
		}
		else RunState.PxlTxtEnabled = false;

		memset(&PxlsInfo, 0, sizeof(MENUITEMINFO));
		PxlsInfo.cbSize = sizeof(MENUITEMINFO);
		PxlsInfo.fMask = MIIM_STATE;
		PxlsInfo.fState = RunState.PxlTxtEnabled ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo(QualityMenu, 4, TRUE, &PxlsInfo);
	}
		break;
		//COLOR
	case ID_COLOR_FULL:
	case ID_COLOR_DUALCOLOR:
	case ID_COLOR_MONOCHROME:
		HandleColorMenu(wParam);
		//DRAW
	case ID_DRAW_REDRAW:
		WindowOriginH = 0;
		WindowOriginW = 0;
		SetScrollPos(hMainWnd, SB_VERT, 0, TRUE);
		SetScrollPos(hMainWnd, SB_HORZ, 0, TRUE);
		ResetWindowSize = true;
		InvalidateRect(hMainWnd, NULL, TRUE);
		break;
	case ID_DRAW_CLEARSCREEN:
	{
		HDC hdc = GetDC(hMainWnd);
		Rectangle(hdc, 0, 0, ScreenWidth, ScreenHeight);
		ReleaseDC(hMainWnd, hdc);
	}
	break;
	case ID_HELP_ABOUT:
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hMainWnd, HelpProc);
		break;
	}
}

void HandleSizeMenu(WPARAM wParam)
{
	MENUITEMINFO SizeMenuInfo;

	memset(&SizeMenuInfo, 0, sizeof(MENUITEMINFO));
	SizeMenuInfo.cbSize = sizeof(MENUITEMINFO);
	SizeMenuInfo.fMask = MIIM_STATE;
	SizeMenuInfo.fState = MFS_UNCHECKED;

	SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 0, true, &SizeMenuInfo); //RATIO
	SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 1, true, &SizeMenuInfo); //FITSCREEN
	SetMenuItemInfo(SizeMenu, 1, true, &SizeMenuInfo); //IMAGESIZE
	SetMenuItemInfo(SizeMenu, 2, true, &SizeMenuInfo); //CUSTOMSIZE
	SizeMenuInfo.fState = MFS_CHECKED;

	switch (LOWORD(wParam))
	{
	case ID_FITSCREEN_RATIO:
		RunState.Size = RunState.ImageSizeOption::PreserveRatio;
		SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 0, true, &SizeMenuInfo);
		break;
	case ID_FITSCREEN_FITSCREEN:
		RunState.Size = RunState.ImageSizeOption::FitScreen;
		SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 1, true, &SizeMenuInfo);
		break;
	case ID_SIZE_IMAGESIZE:
		RunState.Size = RunState.ImageSizeOption::ImageSize;
		SetMenuItemInfo(SizeMenu, 1, true, &SizeMenuInfo);
		break;
	case ID_SIZE_CUSTOMSIZE:
		if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG3), hMainWnd, GetDimensionsProc, ID_SIZE_CUSTOMSIZE) == IDOK)
		{
			RunState.Size = RunState.ImageSizeOption::Custom;
			SetMenuItemInfo(SizeMenu, 2, true, &SizeMenuInfo);
		}

		break;
	}
	return;
}

void HandleQualityMenu(WPARAM wParam)
{
	MENUITEMINFO QualityMenuInfo;

	memset(&QualityMenuInfo, 0, sizeof(MENUITEMINFO));
	QualityMenuInfo.cbSize = sizeof(MENUITEMINFO);
	QualityMenuInfo.fMask = MIIM_STATE;
	QualityMenuInfo.fState = MFS_UNCHECKED;

	SetMenuItemInfo(QualityMenu, 0, true, &QualityMenuInfo);
	SetMenuItemInfo(QualityMenu, 1, true, &QualityMenuInfo);
	SetMenuItemInfo(QualityMenu, 2, true, &QualityMenuInfo);

	QualityMenuInfo.fState = MFS_CHECKED;
	switch (LOWORD(wParam))
	{
	case ID_QUALITY_SIMPLE:
		RunState.Quality = RunState.QualityOption::Simple;
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG4), hMainWnd, GetSimpleDepthFilterProc);
		SetMenuItemInfo(QualityMenu, 0, true, &QualityMenuInfo);
		break;
	case ID_QUALITY_PRECISE:
		RunState.Quality = RunState.QualityOption::Precise;
		SetMenuItemInfo(QualityMenu, 1, true, &QualityMenuInfo);
		break;
	case ID_QUALITY_WEIGHTED:
		RunState.Quality = RunState.QualityOption::Weighted;
		SetMenuItemInfo(QualityMenu, 2, true, &QualityMenuInfo);
		break;
	}
	return;
}

void HandleColorMenu(WPARAM wParam)
{
	MENUITEMINFO ColorMenuInfo;

	memset(&ColorMenuInfo, 0, sizeof(MENUITEMINFO));
	ColorMenuInfo.cbSize = sizeof(MENUITEMINFO);
	ColorMenuInfo.fMask = MIIM_STATE;
	ColorMenuInfo.fState = MFS_UNCHECKED;

	SetMenuItemInfo(ColorMenu, 0, true, &ColorMenuInfo);
	SetMenuItemInfo(ColorMenu, 1, true, &ColorMenuInfo);
	SetMenuItemInfo(ColorMenu, 2, true, &ColorMenuInfo);

	ColorMenuInfo.fState = MFS_CHECKED;

	switch (LOWORD(wParam))
	{
	case ID_COLOR_FULL:
		RunState.Color = true;
		SetMenuItemInfo(ColorMenu, 0, true, &ColorMenuInfo);
		break;
	case ID_COLOR_DUALCOLOR:
		RunState.Color = false;
		DualColor(hMainWnd);
		SetMenuItemInfo(ColorMenu, 1, true, &ColorMenuInfo);
		break;
	case ID_COLOR_MONOCHROME:
		RunState.Color = false;
		TextColor = 0;
		TextBkColor = RGB(255, 255, 255);
		SetMenuItemInfo(ColorMenu, 2, true, &ColorMenuInfo);
		break;
	}
}

void HandleScroll(UINT iMSG, WPARAM wParam)
{
	unsigned int Width = RunState.PxlTxtEnabled ? ::Width*FontHeight / RunState.PxlsPerTxtW : ::Width;
	unsigned int Height = RunState.PxlTxtEnabled ? ::Height*FontHeight / RunState.PxlsPerTxtH : ::Height;

	int DX = 0;
	int DY = 0;


	switch (iMSG)
	{
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN: DY = Height / 100; break; //moves a percent
		case SB_LINEUP:DY = -(int)Height / 100; break;
		case SB_PAGEDOWN:DY = Height / 10; break;
		case SB_PAGEUP:DY = -(int)Height / 10; break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: DY = HIWORD(wParam) - WindowOriginH;	break;
		}
		break;
	case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_LINERIGHT: DX = Width / 100; break;
		case SB_LINELEFT: DX = -(int)Width / 100; break;
		case SB_PAGERIGHT: DX = Width / 10; break;
		case SB_PAGELEFT: DX = -(int)Width / 10; break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			DX = HIWORD(wParam) - WindowOriginW;	break;
			break;
		}
		break;
	}

	SetScrollPos(hMainWnd, SB_HORZ, WindowOriginW += DX, TRUE);
	SetScrollPos(hMainWnd, SB_VERT, WindowOriginH += DY, TRUE);
	InvalidateRect(hMainWnd, NULL, FALSE);
}

void AdjustScrollBarSize()
{
	SCROLLINFO SRI;
	RECT R;

	//SETTING SIZE OF PAGE
	GetClientRect(hMainWnd, &R);
	ZeroMemory(&SRI, sizeof(SCROLLINFO));
	SRI.cbSize = sizeof(SCROLLINFO);
	SRI.fMask = SIF_PAGE;
	SRI.nPage = R.right - R.left;
	SetScrollInfo(hMainWnd, SB_HORZ, &SRI, TRUE);
	SRI.nPage = R.bottom - R.top;
	SetScrollInfo(hMainWnd, SB_VERT, &SRI, TRUE);

	//SETTING RANGE
	ZeroMemory(&SRI, sizeof(SCROLLINFO));
	SRI.fMask = SIF_RANGE;
	SRI.nMin = 0;
	SRI.nMax = OutWidth;
	SetScrollInfo(hMainWnd, SB_HORZ, &SRI, TRUE);
	SRI.nMax = OutHeight;
	SetScrollInfo(hMainWnd, SB_VERT, &SRI, TRUE);
}

void SaveToBitmap()
{
	FILE* OutBitmap;

	//GetFileName
	TCHAR lpstrFile[MAX_PATH] = L"";
	if (SaveBmp(hMainWnd, lpstrFile))
	{
		OutBitmap = _wfopen(lpstrFile, L"wb");
		if (!OutBitmap) return; //ERROR
	}
	else return; //ERROR


	HDC hdc = GetDC(hMainWnd);

	BITMAPFILEHEADER BFH;
	BFH.bfType = (WORD)0x4d42;
	BFH.bfSize = 0;
	BFH.bfReserved2 = 0;
	BFH.bfReserved1 = 0;
	BFH.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPINFOHEADER BIH;
	BIH.biSize = sizeof(BITMAPINFOHEADER);
	BIH.biWidth = OutWidth;
	BIH.biHeight = -(int)OutHeight;
	BIH.biPlanes = 1;
	BIH.biBitCount = 24;
	BIH.biCompression = BI_RGB;
	BIH.biSizeImage = NULL;
	BIH.biXPelsPerMeter = GetDeviceCaps(hdc, HORZRES);
	BIH.biYPelsPerMeter = GetDeviceCaps(hdc, VERTRES);
	BIH.biClrUsed = NULL;
	BIH.biClrImportant = NULL;

	fwrite(&BFH, 1, sizeof(BITMAPFILEHEADER), OutBitmap); //CHECK ERROR
	fwrite(&BIH, 1, sizeof(BITMAPINFOHEADER), OutBitmap); //CHECK ERROR

	BITMAPINFO BI;
	BI.bmiHeader = BIH;

	HDC tmpDC = CreateCompatibleDC(hdc);
	BYTE* memory = NULL;
	HBITMAP hOutBit = CreateDIBSection(hdc, &BI, DIB_RGB_COLORS, (void**)&memory, 0, 0);
	HBITMAP TempBit = (HBITMAP)SelectObject(tmpDC, hOutBit);

	int tmpH = WindowOriginH;
	int tmpW = WindowOriginW;

	WindowOriginH = 0;
	WindowOriginW = 0;
	DrawASCII(tmpDC);
	
	WindowOriginH = tmpH;
	WindowOriginW = tmpW;
	
	//BitBlt(tmpDC, 0, 0, OutWidth, OutHeight, MemDC, 0, 0, SRCCOPY);


	int ByteSize =  (((24 * OutWidth + 31)&(~31)) / 8)*OutHeight;
	fwrite(memory, 1, ByteSize, OutBitmap);
	fflush(OutBitmap);
	fclose(OutBitmap);

	SelectObject(tmpDC, TempBit);
	DeleteDC(tmpDC);
	DeleteObject(hOutBit);

	ReleaseDC(hMainWnd, hdc);


}

void SaveToText()
{
	FILE* OutTextFile;
	TCHAR lpstrFile[MAX_PATH] = L"";
	if (SaveTxt(hMainWnd, lpstrFile))
	{
		OutTextFile = _wfopen(lpstrFile, L"wb");
		if (OutTextFile)
		{
			for (auto &s : OutText)
			{
				fwrite(&s, 1, sizeof(wchar_t), OutTextFile);
			}
			fflush(OutTextFile);
			fclose(OutTextFile);
		}
		else return;
	}
	else return;
}