#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include "resource.h"
#include <map>

#define DefaultFontStyle CreateFont(12,0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_MODERN, L"Courier New")

#define SizeMenu  (GetSubMenu(GetSubMenu(GetMenu(hMainWnd), 1), 0))
#define QualityMenu (GetSubMenu(GetSubMenu(GetMenu(hMainWnd), 1), 1))
#define ColorMenu (GetSubMenu(GetSubMenu(GetMenu(hMainWnd), 1), 2))
enum {WM_EDITVALUE = WM_USER+1};

extern LPCTSTR Title;
struct TextPic
{
	using uint = unsigned int;
	uint N; // Unicode Num
	uint W, H; //Width-Height
	uint cx, cy; //Center Point.
	TextPic(uint N, uint W, uint H, uint cx, uint cy) :N{ N }, W{ W }, H{ H }, cx{ cx }, cy{ cy }{};
	TextPic(){};
};

struct tag_RunState
{
	enum ColorOption{ MonoChrome, RGBColors };
	enum ModeOption{ Register, Render };
	enum QualityOption{ Simple, Precise, Weighted };
	enum ImageSizeOption{ FitScreen, ImageSize, Custom, PreserveRatio };
	bool Color : 1;
	bool Mode : 1;
	unsigned char Quality : 2;
	unsigned char Size : 2;
	bool PxlTxtEnabled : 1;
	bool Pause : 1;
	bool Running : 1;
	unsigned char PxlsPerTxtW : 8;
	unsigned char PxlsPerTxtH : 8;
	unsigned char SimpleDepthFilter : 8;
};
extern HINSTANCE g_hInst;
extern HWND hMainWnd;

//Table I/O
extern void WriteTable(HDC);
extern bool LoadTable();
extern std::multimap<unsigned int, TextPic> DensityMap;
extern bool getFontStyle();

extern TCHAR* FileName(HWND hWnd, TCHAR* lpstrFile,LPCWSTR lpstrFilter);
extern TCHAR* SaveBmp(HWND hWnd, TCHAR* lpstrFile);
extern TCHAR* SaveTxt(HWND hWnd, TCHAR* lpstrFile);

extern void RenderText(HDC& hdc, HBITMAP& SrcBitmap);
extern HBITMAP TargetBitmap;
extern HBITMAP OldBitmap;
extern unsigned int IndexStart;
extern unsigned int IndexEnd;

//DRAWING
extern void DrawASCII(HDC);
extern HDC MemDC;
extern HBITMAP MemBit;
extern HBITMAP OldMemBit;

extern unsigned int Width;
extern unsigned int Height;
extern unsigned int ScreenWidth;
extern unsigned int ScreenHeight;
extern unsigned int CustomWidth;
extern unsigned int CustomHeight;

extern unsigned int OutWidth; //True Width of Image
extern unsigned int OutHeight;

extern LOGFONT LF;
extern HFONT hFont;
extern unsigned int FontHeight;
extern void DualColor(HWND);
extern COLORREF TextColor;
extern COLORREF TextBkColor;
extern bool ResetWindowSize;

//Initializing Dialog && Options
extern BOOL CALLBACK OptionDlgProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK HelpProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK GetDimensionsProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK GetSimpleDepthFilterProc(HWND, UINT, WPARAM, LPARAM);
extern tag_RunState RunState;


// Pausing / Receiving Message While Rendering
extern int CheckForMessage();

//Text Version
extern std::wstring OutText;