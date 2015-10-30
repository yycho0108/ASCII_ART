#include "Header.h"

void RenderTextInitialize(HDC&, HBITMAP&);
using uint = unsigned int;

HBITMAP MemBit;
HBITMAP OldMemBit;
uint Width;
uint Height;
bool ResetWindowSize;
void DisplayState(uint, uint);

void RenderPerRead(HDC&, HDC&);
void RenderPerPixel(HDC&, HDC&);

std::wstring OutText;

void RenderText(HDC& hdc, HBITMAP& SrcBitmap)
{
	HDC MDC = CreateCompatibleDC(hdc);
	HBITMAP OldBit = (HBITMAP)SelectObject(MDC, SrcBitmap);
	RenderTextInitialize(hdc, SrcBitmap);


	if (!RunState.PxlTxtEnabled) RenderPerRead(hdc, MDC);
	else RenderPerPixel(hdc, MDC);

	SetWindowText(hMainWnd, Title);
	SelectObject(MDC, OldBit);
	DeleteDC(MDC);
	ResetWindowSize = true;
	InvalidateRect(hMainWnd, NULL, TRUE);
	RunState.Running = false;
}

void RenderTextInitialize(HDC& hdc, HBITMAP& SrcBitmap)
{
	BITMAP Bit;
	GetObject(SrcBitmap, sizeof(BITMAP), &Bit);
	::Width = Bit.bmWidth;
	::Height = Bit.bmHeight; //GLOBALS

	unsigned int Width = RunState.PxlTxtEnabled ? ::Width*FontHeight / RunState.PxlsPerTxtW : ::Width;
	unsigned int Height = RunState.PxlTxtEnabled ? ::Height*FontHeight / RunState.PxlsPerTxtH : ::Height;
	//Creates "LOCAL WIDTH" == TRUEWIDTH (OF IMAGE)

	SetScrollRange(hMainWnd, SB_HORZ, 0, Width, TRUE);
	SetScrollRange(hMainWnd, SB_VERT, 0, Height, TRUE);
	SetScrollPos(hMainWnd, SB_HORZ, 0, TRUE);
	SetScrollPos(hMainWnd, SB_VERT, 0, TRUE);
	//Set Scrolls
	MemBit = CreateCompatibleBitmap(hdc, Width, Height);

	OldMemBit = (HBITMAP)SelectObject(MemDC, MemBit);
	SelectObject(MemDC, hFont);
	SelectObject(hdc, hFont);

	SetTextColor(MemDC, TextColor);
	SetBkColor(MemDC, TextBkColor);
	SetTextColor(hdc, TextColor);
	SetBkColor(hdc, TextBkColor);
	HBRUSH CurBrush = CreateSolidBrush(TextBkColor);
	HBRUSH OldBrush = (HBRUSH)SelectObject(hdc, CurBrush);
	Rectangle(hdc, 0, 0, Width, Height);
	SelectObject(hdc, OldBrush);
	HBRUSH OldBrush_2 = (HBRUSH)SelectObject(MemDC, CurBrush);
	Rectangle(MemDC, 0, 0, Width, Height);
	SelectObject(MemDC, OldBrush_2);
	DeleteObject(CurBrush);

	//
	OutText.clear();
	OutText.reserve((Width*Height)/(FontHeight*FontHeight));
}

int CheckForMessage()
{
	MSG Msg;
	if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (Msg.message == WM_QUIT)
		{
			PostQuitMessage(0);
			return -1; // -1 = QUIT.
		}
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	if (!RunState.Running) return -1; //Running is False
	if (RunState.Pause) return 1;
	return 0;
}

void DisplayState(uint i, uint j)
{
	TCHAR strbuf[256];
	wsprintf(strbuf, L"PROGRESS : %u%%([%u,%u] /[%u,%u])", (100*(i*Width+j))/(Height*Width),i, j, Height, Width);
	SetWindowText(hMainWnd, strbuf);
}

void RenderPerRead(HDC& hdc, HDC& MDC)
{
	for (uint i = 0; i < Height; i += FontHeight)
	{
		for (uint j = 0; j < Width; j)
		{
			int CM = CheckForMessage();
			if (CM == -1) return;
			else if (CM == 1) continue;
			unsigned int RAve = 0, GAve = 0, BAve = 0;
			unsigned int CX = 0, CY = 0;
			unsigned int TotVal = 0;
			DisplayState(i, j);
			for (uint r_i = 0; r_i < FontHeight; r_i++)
			{
				for (uint r_j = 0; r_j < 20; r_j++)
				{
					COLORREF C = GetPixel(MDC, j + r_j, i + r_i);
					RAve += GetRValue(C);
					GAve += GetGValue(C);
					BAve += GetBValue(C);

					if (RunState.Quality == RunState.QualityOption::Weighted)
					{
						unsigned int WeightVal = (GetRValue(C)*0.23 + GetGValue(C)*0.07 + GetBValue(C)*0.70);
						TotVal += WeightVal;
						CX += r_j*WeightVal;
						CY += r_i*WeightVal;
					}

				}
			}

			RAve /= 20 * FontHeight; //Averaged
			GAve /= 20 * FontHeight;
			BAve /= 20 * FontHeight;

			if (RunState.Color == RunState.ColorOption::RGBColors)
			{
				SetTextColor(hdc, RGB(RAve, GAve, BAve));
				SetTextColor(MemDC, RGB(RAve, GAve, BAve));
				SetBkColor(hdc, RGB(255, 255, 255));
				SetBkColor(MemDC, RGB(255, 255, 255));
			}

			if (RunState.Quality == RunState.QualityOption::Weighted && TotVal)
			{
				CX /= TotVal;
				CY /= TotVal;
			}


			uint GrayVal = 0.21*RAve + 0.72*GAve + 0.07*BAve; //Luminosity

			if (RunState.Quality != RunState.QualityOption::Simple)
			{
				int propagate = 0;
				bool sign = false;

				while (!DensityMap.count(GrayVal))
				{
					GrayVal += (++propagate)*((sign = !sign) ? 1 : -1);
				}
				auto c = DensityMap.count(GrayVal);
				auto h = DensityMap.lower_bound(GrayVal);

				if (RunState.Quality == RunState.QualityOption::Weighted)	//find best-fit
				{
					auto bestfit = std::make_pair(h, MAXUINT);

					for (auto W_i = h; c--; ++W_i)
					{
						uint C_val = (W_i->second.cx - CX)*(W_i->second.cx - CX) + (W_i->second.cy - CY)*(W_i->second.cy - CY);
						if (bestfit.second > C_val)
							bestfit.first = W_i, bestfit.second = C_val;
					}
					h = bestfit.first;
				}
				else std::advance(h, rand() % c);//find random fit


				TCHAR charbuf[2] = { static_cast<wchar_t>(h->second.N), 0 };
				TextOut(MemDC, j, i, charbuf, 1);
				TextOut(hdc, j, i, charbuf, 1);
				OutText += charbuf;
				j += h->second.W; //Width - offset.
			}
			else //if (RunState.Quality == RunState.QualityOption::Simple)
			{
				TCHAR Char_is[] = L"1";
				TCHAR Char_not[] = L" ";
				TextOut(hdc, j, i, GrayVal < RunState.SimpleDepthFilter? Char_is : Char_not, 1);
				TextOut(MemDC, j, i, GrayVal < RunState.SimpleDepthFilter ? Char_is : Char_not, 1);
				OutText += GrayVal < RunState.SimpleDepthFilter ? Char_is : Char_not;
				SIZE SZ;
				GetTextExtentExPointW(hdc, Char_is, 1, 50, NULL, NULL, &SZ);
				j += SZ.cx;
			}
		}
		OutText += L"\r\n";
	}
}
void RenderPerPixel(HDC& hdc, HDC& MDC)
{
	for (uint i = 0, out_i = 0; i < Height; i += RunState.PxlsPerTxtH, out_i += FontHeight)
	{
		for (double j = 0, out_j = 0; j < Width;)
		{
			int CM = CheckForMessage();
			if (CM == -1) return;
			else if (CM == 1) continue;

			DisplayState(i, j);
			uint RAve = 0, GAve = 0, BAve = 0;
			uint CX = 0, CY = 0;
			uint TotVal = 0;

			for (int r_i = 0; r_i < RunState.PxlsPerTxtH; r_i++)
			{
				for (int r_j = 0; r_j < RunState.PxlsPerTxtW; r_j++)
				{
					COLORREF C = GetPixel(MDC, j + r_j, i + r_i);
					RAve += GetRValue(C);
					GAve += GetGValue(C);
					BAve += GetBValue(C);

					if (RunState.Quality == RunState.QualityOption::Weighted)
					{
						unsigned int WeightVal = (GetRValue(C)*0.23 + GetGValue(C)*0.07 + GetBValue(C)*0.70);
						TotVal += WeightVal;
						CX += r_j*WeightVal;
						CY += r_i*WeightVal;
					}

				}
			}

			RAve /= RunState.PxlsPerTxtW*RunState.PxlsPerTxtH;
			GAve /= RunState.PxlsPerTxtW*RunState.PxlsPerTxtH;
			BAve /= RunState.PxlsPerTxtW*RunState.PxlsPerTxtH;

			if (RunState.Color == RunState.ColorOption::RGBColors)
			{
				SetTextColor(hdc, RGB(RAve, GAve, BAve));
				SetTextColor(MemDC, RGB(RAve, GAve, BAve));
				SetBkColor(hdc, RGB(255, 255, 255));
				SetBkColor(MemDC, RGB(255, 255, 255));
			}

			if (RunState.Quality == RunState.QualityOption::Weighted && TotVal)
			{
				CX /= TotVal;
				CY /= TotVal;
			}


			int GrayVal = 0.21*RAve + 0.72*GAve + 0.07*BAve; //Luminosity

			if (RunState.Quality != RunState.QualityOption::Simple)
			{
				int propagate = 0;
				bool sign = false;

				while (!DensityMap.count(GrayVal))
				{
					GrayVal += (++propagate)*((sign = !sign) ? 1 : -1);
				}
				auto c = DensityMap.count(GrayVal);
				auto h = DensityMap.lower_bound(GrayVal);

				if (RunState.Quality == RunState.QualityOption::Weighted)	//find best-fit
				{
					auto bestfit = std::make_pair(h, MAXUINT);

					for (auto W_i = h; c--; ++W_i)
					{
						unsigned int C_val = (W_i->second.cx - CX)*(W_i->second.cx - CX) + (W_i->second.cy - CY)*(W_i->second.cy - CY);
						if (bestfit.second > C_val)
							bestfit.first = W_i, bestfit.second = C_val;
					}
					h = bestfit.first;
				}
				else std::advance(h, rand() % c);//find random fit


				TCHAR charbuf[2] = { static_cast<wchar_t>(h->second.N), 0 };
				TextOut(MemDC, out_j, out_i, charbuf, 1);
				TextOut(hdc, out_j, out_i, charbuf, 1);
				OutText += static_cast<wchar_t>(h->second.N);
				out_j += h->second.W; //Width - offset.
				//j += max(h->second.W*RunState.PxlsPerTxtW / FontHeight, 1);
				j += (h->second.W*RunState.PxlsPerTxtW*1.0)/ FontHeight;
			}
			else //if (RunState.Quality == RunState.QualityOption::Simple)
			{
				TCHAR Char_is[] = L"1";
				TCHAR Char_not[] = L" ";
				TextOut(hdc, out_j, out_i, GrayVal < 150 ? Char_is : Char_not, 1);
				TextOut(MemDC, out_j, out_i, GrayVal < 150 ? Char_is : Char_not, 1);
				OutText += GrayVal < 150 ? Char_is : Char_not;
				SIZE SZ;
				GetTextExtentExPointW(hdc, Char_is, 1, 50, NULL, NULL, &SZ);
				out_j += SZ.cx;
				j += (SZ.cx*RunState.PxlsPerTxtW*1.0) / FontHeight;
			}
		}

		OutText += L"\r\n";
	}
}