#include "Header.h"
#include <commctrl.h>
#pragma comment (lib, "ComCtl32.lib")

HWND hMainDlg;


tag_RunState RunState = {};
HFONT hFont;
COLORREF TextColor;
COLORREF TextBkColor;

unsigned int CustomWidth = 500;
unsigned int CustomHeight = 500;
unsigned int IndexStart = 0;
unsigned int IndexEnd = 65536;


static inline void SetMode(HWND hDlg,BOOL STATE);
static inline void GetMode(HWND hDlg);
static inline void GetImageSize(HWND hDlg);
static inline void GetImageQuality(HWND hDlg);
static inline bool GetDimensions(HWND hDlg);
void DualColor(HWND);

void SetupMenuOptions();


HWND hToolTip;
void RegisterTooltip(HWND hDlg);

BOOL CALLBACK OptionDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
	case WM_INITDIALOG:
	{
		hMainDlg = hDlg;
	
		if (!TextColor&&!TextBkColor) TextColor = 0, TextBkColor = RGB(255, 255, 255);
		SendDlgItemMessage(hDlg, IDC_CHECK1, BM_SETCHECK, RunState.Color?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_RADIO1, BM_SETCHECK, RunState.Quality==RunState.QualityOption::Precise?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_RADIO2, BM_SETCHECK, RunState.Quality == RunState.QualityOption::Weighted ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_RADIO3, BM_SETCHECK, RunState.Quality == RunState.QualityOption::Simple ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, RunState.Mode?IDC_RADIO5:IDC_RADIO4, BM_SETCHECK, BST_CHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_RADIO7, BM_SETCHECK, BST_CHECKED, 0);
		SetMode(hDlg, !RunState.Mode);
		SetDlgItemInt(hDlg, IDC_EDIT1,CustomWidth, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT2,CustomHeight, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT3,IndexStart, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT4,IndexEnd, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT5, RunState.SimpleDepthFilter, FALSE);

		RegisterTooltip(hDlg);
		break;
	}
	case WM_NOTIFY:
	{
		if (wParam == TTN_SHOW)
		{
			MessageBox(hDlg, L"??", L"!!", MB_OK);
			MessageBeep(MB_OK);
			if (((NMHDR*)lParam)->code == TTN_SHOW)
			{
				MessageBeep(MB_OK);
			}
		}
		break;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO3:
			if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG4), hDlg, GetSimpleDepthFilterProc)==IDOK)
				SetDlgItemInt(hDlg, IDC_EDIT5, RunState.SimpleDepthFilter, FALSE);
			break;
		case IDC_RADIO4:
			SetMode(hDlg, TRUE);
			break;
		case IDC_RADIO5:
			SetMode(hDlg, FALSE);
			break;
		case IDC_RADIO6:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK2), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
			break;
		case IDC_RADIO7:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK2), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
			break;
		case IDC_RADIO8:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK2), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), TRUE);
			break;
		case IDC_BUTTON1:
		{
			DualColor(hDlg);
			break;
		}
		case IDC_BUTTON2:
		{
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hDlg, HelpProc);
			break;
		}
		case IDC_BUTTON3:
		{
			if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG3), hDlg, GetDimensionsProc,ID_QUALITY_PXLS) == IDOK)
				RunState.PxlTxtEnabled = true;
			else RunState.PxlTxtEnabled = false;
			break;
		}
		case IDOK:
			//CHECK ALL SORTS OF SHIT...
			GetMode(hDlg);
			GetImageSize(hDlg);
			GetImageQuality(hDlg);
			if (SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0) == BST_CHECKED)
				RunState.Color = RunState.ColorOption::RGBColors;
			else RunState.Color = RunState.ColorOption::MonoChrome;

			if (GetDimensions(hDlg))
			{
				SetupMenuOptions();
				EndDialog(hDlg, IDOK);
				 if (RunState.Mode == RunState.ModeOption::Register)
					 getFontStyle();
				 else LoadTable();
				return TRUE;
			}
			else
			{
				MessageBox(hDlg, L"Wrong Value", L"ERROR", MB_OK);
				EndDialog(hDlg, IDCANCEL);
			}
				
			break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		default:
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void SetMode(HWND hDlg,BOOL STATE)
{
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO1), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO2), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO3), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO6), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO7), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_RADIO8), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), !STATE&&SendDlgItemMessage(hDlg, IDC_RADIO6, BM_GETCHECK, 0, 0) == BST_CHECKED);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), !STATE&&SendDlgItemMessage(hDlg, IDC_RADIO6, BM_GETCHECK, 0, 0) == BST_CHECKED);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK2), !STATE&&SendDlgItemMessage(hDlg, IDC_RADIO8, BM_GETCHECK,0, 0) == BST_CHECKED);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), !STATE&&SendDlgItemMessage(hDlg, IDC_CHECK1, BM_GETCHECK, 0, 0) != BST_CHECKED);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON3), !STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT3), STATE);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT4), STATE);
}

void GetMode(HWND hDlg)
{
	if (SendDlgItemMessage(hDlg, IDC_RADIO4, BM_GETCHECK, 0, 0) == BST_CHECKED)
		RunState.Mode = RunState.ModeOption::Register;
	else 
		RunState.Mode = RunState.ModeOption::Render;
}

void GetImageSize(HWND hDlg)
{
	if (SendDlgItemMessage(hDlg, IDC_RADIO6, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		if ((SendDlgItemMessage(hDlg, IDC_CHECK2, BM_GETCHECK, 0, 0) == BST_CHECKED))
			RunState.Size = RunState.ImageSizeOption::PreserveRatio;
		else RunState.Size = RunState.ImageSizeOption::FitScreen;
	}
	else if ((SendDlgItemMessage(hDlg, IDC_RADIO7, BM_GETCHECK, 0, 0) == BST_CHECKED))
		RunState.Size = RunState.ImageSizeOption::ImageSize;
	else //if ((SendDlgItemMessage(hDlg, IDC_RADIO8, BM_GETCHECK, 0, 0) == BST_CHECKED))
		RunState.Size = RunState.ImageSizeOption::Custom;

	return;
}

void GetImageQuality(HWND hDlg)
{
	if ((SendDlgItemMessage(hDlg, IDC_RADIO3, BM_GETCHECK, 0, 0) == BST_CHECKED))
		RunState.Quality=RunState.QualityOption::Simple;
	else if ((SendDlgItemMessage(hDlg, IDC_RADIO1, BM_GETCHECK, 0, 0) == BST_CHECKED))
		RunState.Quality = RunState.QualityOption::Precise;
	else if ((SendDlgItemMessage(hDlg, IDC_RADIO2, BM_GETCHECK, 0, 0) == BST_CHECKED))
		RunState.Quality = RunState.QualityOption::Weighted;
	return;
}

bool GetDimensions(HWND hDlg)
{
	BOOL w, h, s, e;

	if (RunState.Mode == RunState.ModeOption::Render && RunState.Size == RunState.ImageSizeOption::Custom)
	{
			CustomWidth = GetDlgItemInt(hDlg, IDC_EDIT1, &w, FALSE);
			CustomHeight = GetDlgItemInt(hDlg, IDC_EDIT2, &h, FALSE);
	}
	else
		w = h = true;
	IndexStart = GetDlgItemInt(hDlg, IDC_EDIT3, &s, FALSE);
	IndexEnd = GetDlgItemInt(hDlg, IDC_EDIT4, &e, FALSE);

	return w&&h&&s&&e&&IndexStart < IndexEnd;
}

BOOL CALLBACK HelpProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (iMsg == WM_COMMAND && LOWORD(wParam) == IDOK)
	{
		EndDialog(hDlg, IDOK);
		return TRUE;
	}
	else return FALSE;
}

void DualColor(HWND hWndOwner)
{
	CHOOSECOLOR COL;
	static COLORREF lpCustColors[16];
	memset(&COL, 0, sizeof(CHOOSECOLOR));
	COL.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
	COL.hwndOwner = hWndOwner;
	COL.lStructSize = sizeof(CHOOSECOLOR);
	COL.lpCustColors = lpCustColors;
	MessageBox(hWndOwner, L"Choose Text Color", L"Text", MB_OK);
	if (ChooseColor(&COL))TextColor = COL.rgbResult;
	MessageBox(hWndOwner, L"Choose Text Background Color", L"Background", MB_OK);
	if (ChooseColor(&COL))TextBkColor = COL.rgbResult;
}

bool getFontStyle()
{
	CHOOSEFONT CF;
	memset(&CF, 0, sizeof(CHOOSEFONT));
	CF.lStructSize = sizeof(CHOOSEFONT);
	CF.hwndOwner = hMainWnd;
	CF.lpLogFont = &LF;
	CF.nSizeMin = 5;
	CF.nSizeMax = 100;
	CF.Flags = CF_SCALABLEONLY | CF_BOTH | CF_LIMITSIZE;

	if (ChooseFont(&CF))
	{
		HDC hdc = GetDC(hMainWnd);
		if (LF.lfHeight > 0) LF.lfHeight = -LF.lfHeight;
		hFont = CreateFontIndirect(&LF);
		FontHeight = abs(LF.lfHeight);
		ReleaseDC(hMainWnd, hdc);
		return true;
	}
	else { hFont = DefaultFontStyle; return false; }
}


BOOL CALLBACK GetDimensionsProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static LPARAM flag;
	if (iMsg == WM_INITDIALOG)
	{
		flag = lParam;
		if (flag == ID_QUALITY_PXLS)
		{
			SetWindowText(hDlg, L"Pixel Dimensons");
			SetDlgItemInt(hDlg, IDC_EDIT1, RunState.PxlsPerTxtW, FALSE);
			SetDlgItemInt(hDlg, IDC_EDIT2, RunState.PxlsPerTxtH, FALSE);
		}
		else if (flag == ID_SIZE_CUSTOMSIZE)
		{
			SetWindowText(hDlg, L"Custom Size Dimensions");
			SetDlgItemInt(hDlg, IDC_EDIT1, CustomWidth, FALSE);
			SetDlgItemInt(hDlg, IDC_EDIT2, CustomHeight, FALSE);
		}
	}
	if (iMsg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDOK)
		{
			BOOL w, h;
			unsigned int tmpw, tmph;
			tmpw = GetDlgItemInt(hDlg, IDC_EDIT1, &w, FALSE);
			tmph = GetDlgItemInt(hDlg, IDC_EDIT2, &h, FALSE);

			if (flag == ID_QUALITY_PXLS)
			{
				if (w&&h)
				{
					RunState.PxlsPerTxtW = tmpw;
					RunState.PxlsPerTxtH = tmph;
					RunState.PxlTxtEnabled = true;
				}
				else
				{
					RunState.PxlTxtEnabled = false;
					MessageBox(hMainWnd, L"Wrong Pixel Size", L"ERROR", MB_OK);
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
				}
			}
			else if (flag == ID_SIZE_CUSTOMSIZE)
			{
				if (w&&h)
				{
					CustomWidth = tmpw;
					CustomHeight = tmph;
					RunState.Size = RunState.ImageSizeOption::Custom;
				}
				else
				{
					MessageBox(hMainWnd, L"Wrong Dimensions; Set to (1,1)", L"ERROR", MB_OK);
					CustomWidth = 1;
					CustomHeight = 1;
					RunState.Size = RunState.ImageSizeOption::Custom;
				}

				TCHAR strbuf[100];
				wsprintf(strbuf, L"CS[%uX%u]", CustomWidth, CustomHeight);

				MENUITEMINFO SizeInfo;
				memset(&SizeInfo, 0, sizeof(MENUITEMINFO));
				SizeInfo.cbSize = sizeof(MENUITEMINFO);
				SizeInfo.fMask = MIIM_STRING;
				SizeInfo.dwTypeData = strbuf;
				SetMenuItemInfo(SizeMenu, 2, true, &SizeInfo);
			}
			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	return FALSE;
}

void SetupMenuOptions()
{
	MENUITEMINFO ResetInfo;
	memset(&ResetInfo, 0, sizeof(MENUITEMINFO));
	ResetInfo.cbSize = sizeof(MENUITEMINFO);
	ResetInfo.fMask = MIIM_STATE;
	ResetInfo.fState = MFS_UNCHECKED;
	SetMenuItemInfo(QualityMenu, 0, true, &ResetInfo);
	SetMenuItemInfo(QualityMenu, 1, true, &ResetInfo);
	SetMenuItemInfo(QualityMenu, 2, true, &ResetInfo);
	SetMenuItemInfo(QualityMenu, 4, true, &ResetInfo);

	SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 0, true, &ResetInfo); //RATIO
	SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 1, true, &ResetInfo); //FITSCREEN
	SetMenuItemInfo(SizeMenu, 1, true, &ResetInfo); //IMAGESIZE
	SetMenuItemInfo(SizeMenu, 2, true, &ResetInfo); //CUSTOMSIZE

	SetMenuItemInfo(ColorMenu, 0, true, &ResetInfo);
	SetMenuItemInfo(ColorMenu, 1, true, &ResetInfo);
	SetMenuItemInfo(ColorMenu, 2, true, &ResetInfo);

	ResetInfo.fState = MFS_CHECKED;


	switch (RunState.Color)
	{
	case RunState.ColorOption::MonoChrome:
		if (TextColor == 0 && TextBkColor == RGB(255, 255, 255)) SetMenuItemInfo(ColorMenu, 2, true, &ResetInfo);
		else SetMenuItemInfo(ColorMenu, 1, true, &ResetInfo);
		break;
	case RunState.ColorOption::RGBColors:
		SetMenuItemInfo(ColorMenu, 0, true, &ResetInfo);
		break;
	}

	switch (RunState.Quality)
	{
	case RunState.QualityOption::Simple:
		SetMenuItemInfo(QualityMenu, 0, true, &ResetInfo);
		break;
	case RunState.QualityOption::Precise:
		SetMenuItemInfo(QualityMenu, 1, true, &ResetInfo);
		break;
	case RunState.QualityOption::Weighted:
		SetMenuItemInfo(QualityMenu, 2, true, &ResetInfo);
		break;
	}

	switch (RunState.Size)
	{
	case RunState.ImageSizeOption::PreserveRatio:
		SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 0, true, &ResetInfo);
		break;
	case RunState.ImageSizeOption::FitScreen:
		SetMenuItemInfo(GetSubMenu(SizeMenu, 0), 1, true, &ResetInfo);
		break;
	case RunState.ImageSizeOption::ImageSize:
		SetMenuItemInfo(SizeMenu, 1, true, &ResetInfo);
		break;
	case RunState.ImageSizeOption::Custom:
		SetMenuItemInfo(SizeMenu, 2, true, &ResetInfo);
		break;
	}

	//PXLS

	ResetInfo.fState = RunState.PxlTxtEnabled ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(QualityMenu, 4, TRUE, &ResetInfo);

	if (RunState.PxlTxtEnabled)
	{
		TCHAR strbuf[100];
		ResetInfo.fMask = MIIM_STRING;
		ResetInfo.fState = 0;
		wsprintf(strbuf, L"Pxl/Txt[%uX%u]", RunState.PxlsPerTxtW, RunState.PxlsPerTxtH);
		ResetInfo.dwTypeData = strbuf;
		SetMenuItemInfo(QualityMenu, 4, TRUE, &ResetInfo);
	}

}


BOOL CALLBACK GetSimpleDepthFilterProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
		SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, RunState.SimpleDepthFilter);
		SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETLINESIZE, TRUE, 1);
		SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPAGESIZE, TRUE, 5);
		SetDlgItemInt(hDlg, IDC_EDIT1, RunState.SimpleDepthFilter, FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		default:
			return false;
		}
		break;
	case WM_HSCROLL:
	{
		unsigned int CurPos = SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_GETPOS, 0, 0);
		switch (LOWORD(wParam))
		{
		case TB_LINEDOWN: SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, --CurPos); break;
		case TB_LINEUP:SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, ++CurPos); break;
		case TB_PAGEDOWN:SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, CurPos -= 5); break;
		case TB_PAGEUP:SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, CurPos += 5); break;
		case TB_THUMBTRACK:SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, CurPos = HIWORD(wParam)); break;
		}
		if (CurPos < 0) CurPos = 0;
		else if (CurPos > 255) CurPos = 255;
		SetDlgItemInt(hDlg, IDC_EDIT1, RunState.SimpleDepthFilter = CurPos, FALSE);
		break;
	}
	default:
		return false;
	}
	return true;
}

void RegisterTooltip(HWND hDlg)
{
	INITCOMMONCONTROLSEX ICCE;
	ICCE.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ICCE.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&ICCE);


	hToolTip = CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,TOOLTIPS_CLASS,
		NULL, WS_CHILD | WS_POPUP | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, NULL, g_hInst, 0);

	TOOLINFO TIF = {};
	TIF.cbSize = TTTOOLINFOW_V1_SIZE;
	TIF.hinst = g_hInst;
	TIF.hwnd = hDlg;
	TIF.uFlags = TTF_SUBCLASS | TTF_IDISHWND;

	TIF.uId = (UINT_PTR)GetDlgItem(hDlg, IDC_RADIO4);
	TIF.lpszText = (LPTSTR)IDS_STRING108;
	SendMessage(hToolTip, TTM_ADDTOOL, 0, (LPARAM)&TIF);

	TIF.uId = (UINT_PTR)GetDlgItem(hDlg, IDC_RADIO5);
	TIF.lpszText = (LPTSTR)IDS_STRING109;
	SendMessage(hToolTip, TTM_ADDTOOL, 0, (LPARAM)&TIF);
}