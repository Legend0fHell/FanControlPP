// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#include "tray.h"

HDC _hdc, _hdc_mask;
HBITMAP _hbitmap, _hbitmap_mask;
HFONT _hfont;
HICON _hicon;
RECT _icon_size;

int tray_icon_number = 0;
COLORREF tray_icon_clr;

const COLORREF CLR_ECO = RGB(198, 255, 0);
const COLORREF CLR_BALANCED = RGB(64, 196, 255);
const COLORREF CLR_TURBO = RGB(255, 82, 82);
const COLORREF CLR_DISABLED = RGB(224, 224, 224);
const COLORREF CLR_WHITE = RGB(255, 255, 255);
const COLORREF CLR_BLACK = RGB(0, 0, 0);

// get dpi value using dpix and dpiy
LONG GetDPIValue(HDC hdc, RECT* rect) {
	LONG dpi = 0;

	if (hdc == NULL)
		hdc = GetDC(NULL);

	if (hdc != NULL)
	{
		dpi = GetDeviceCaps(hdc, LOGPIXELSX);

		if (rect != NULL)
		{
			rect->left = MulDiv(rect->left, dpi, 96);
			rect->top = MulDiv(rect->top, dpi, 96);
			rect->right = MulDiv(rect->right, dpi, 96);
			rect->bottom = MulDiv(rect->bottom, dpi, 96);
		}

		ReleaseDC(NULL, hdc);
	}

	return dpi;
}

LONG GetTaskbarDPI() {
	APPBARDATA taskbar_rect = { 0 };

	taskbar_rect.cbSize = sizeof(taskbar_rect);

	if (SHAppBarMessage(ABM_GETTASKBARPOS, &taskbar_rect))
		return GetDPIValue(NULL, &taskbar_rect.rc);

	return GetDPIValue(NULL, NULL);
}

void FontInit(PLOGFONT logfont) {
	RtlZeroMemory(logfont, sizeof(LOGFONT));

	inipp::Ini<wchar_t> settings;
	read_settings(settings);
	std::wstring font_face_name = L"Arial";
	int tray_icon_font_size = 48;
	inipp::extract(settings.sections[L"General"][L"TrayIconFont"], font_face_name);
	inipp::extract(settings.sections[L"General"][L"TrayIconFontSize"], tray_icon_font_size);
	settings.clear();

	wcscpy_s(logfont->lfFaceName, font_face_name.c_str());
	logfont->lfHeight = -(LONG)(tray_icon_font_size * GetTaskbarDPI()) / 96;
	logfont->lfWeight = FW_BOLD;
	logfont->lfCharSet = DEFAULT_CHARSET;
	logfont->lfQuality = CLEARTYPE_QUALITY;
}

HBITMAP CreateAlphaBitmap(HDC hdc, LONG width, LONG height, _Out_ _When_(return != NULL, _Notnull_) void** bits) {
	BITMAPINFO bmi = { 0 };
	HBITMAP hbitmap;
	HDC new_hdc = NULL;
	if (!hdc)
	{
		new_hdc = GetDC(NULL);
		if (!new_hdc)
		{
			*bits = NULL;
			return NULL;
		}
		hdc = new_hdc;
	}

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, bits, NULL, 0);
	if (new_hdc)
		ReleaseDC(NULL, new_hdc);

	return hbitmap;
}

BOOL TrayIconInit() {
	LOGFONT logfont;
	PVOID bits;

	HDC hdcOri;
	FontInit(&logfont);
	_hfont = CreateFontIndirectW(&logfont);

	inipp::Ini<wchar_t> settings;
	read_settings(settings);
	int tray_icon_size = 64;
	inipp::extract(settings.sections[L"General"][L"TrayIconSize"], tray_icon_size);
	settings.clear();

	SetRect(&_icon_size, 0, 0, tray_icon_size, tray_icon_size);
	hdcOri = GetDC(NULL);

	if (!hdcOri) {
		_dErr(L"GetDC failed!");
		return FALSE;
	}

	_hdc = CreateCompatibleDC(hdcOri);
	_hdc_mask = CreateCompatibleDC(hdcOri);

	// Create bitmap
	_hbitmap = CreateAlphaBitmap(_hdc, _icon_size.right, _icon_size.bottom, &bits);
	_hbitmap_mask = CreateBitmap(_icon_size.right, _icon_size.bottom, 1, 1, NULL);

	ReleaseDC(NULL, hdcOri);
	return TRUE;
}

// draw background on hdc
void DrawBackground(HDC & hdc, COLORREF bg_clr, COLORREF pen_clr, COLORREF brush_clr) {
	HGDIOBJ prev_brush;
	HGDIOBJ prev_pen;
	COLORREF prev_clr;

	prev_brush = SelectObject(hdc, GetStockObject(DC_BRUSH));
	prev_pen = SelectObject(hdc, GetStockObject(DC_PEN));

	prev_clr = SetBkColor(hdc, bg_clr);

	SetDCPenColor(hdc, pen_clr);
	SetDCBrushColor(hdc, brush_clr);

	FillRect(hdc, &_icon_size, (HBRUSH)GetStockObject(bg_clr));
	SelectObject(hdc, prev_brush);
	SelectObject(hdc, prev_pen);
	SetBkColor(hdc, prev_clr);
}

void UpdateText(HDC & hdc, wchar_t* szText, int text_length, PRECT rect, COLORREF clr) {
	COLORREF prev_clr;
	ULONG flags;

	prev_clr = SetTextColor(hdc, clr);

	flags = DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX;

	DrawTextExW(hdc, szText, text_length, rect, flags, NULL);

	SetTextColor(hdc, prev_clr);
}

BOOL UpdateTrayIconNumber(COLORREF clr, const wchar_t* str, const int num) {
	wchar_t* szText = NULL;
	if (str != NULL) szText = _wcsdup(str);
	else {
		if (tray_icon_clr == clr && tray_icon_number == num) {
			return TRUE; // if the number is the same, no need to update
		}
		tray_icon_number = num;
		szText = _wcsdup(_ts(tray_icon_number).c_str());
		_dInfo(_ts(L"[UpdateTrayIconNumber] Thermal Updating Icon: ") + _ts(tray_icon_number));
	}

	tray_icon_clr = clr;

	HGDIOBJ prev_bmp, prev_font;
	ICONINFO iconInfo{};
	int prev_mode;

	prev_bmp = SelectObject(_hdc, _hbitmap);
	prev_font = SelectObject(_hdc, _hfont);
	prev_mode = SetBkMode(_hdc, TRANSPARENT);

	DrawBackground(_hdc, TRANSPARENT, TRANSPARENT, TRANSPARENT);

	UpdateText(_hdc, szText, lstrlen(szText), &_icon_size, clr);

	SetBkMode(_hdc, prev_mode);
	SelectObject(_hdc, prev_font);
	SelectObject(_hdc, prev_bmp);

	// Draw mask
	prev_bmp = SelectObject(_hdc_mask, _hbitmap_mask);
	prev_font = SelectObject(_hdc_mask, _hfont);
	prev_mode = SetBkMode(_hdc_mask, TRANSPARENT);

	DrawBackground(_hdc_mask, CLR_WHITE, CLR_WHITE, CLR_WHITE);

	UpdateText(_hdc_mask, szText, lstrlen(szText), &_icon_size, CLR_BLACK);

	SetBkMode(_hdc_mask, prev_mode);

	SelectObject(_hdc_mask, prev_bmp);
	SelectObject(_hdc_mask, prev_font);

	iconInfo.fIcon = TRUE;
	iconInfo.hbmColor = _hbitmap;
	iconInfo.hbmMask = _hbitmap_mask;

	HICON hicon_new = CreateIconIndirect(&iconInfo);

	if (_hicon)
		DestroyIcon(_hicon);

	_hicon = hicon_new;

	free(szText);

	return TRUE;
}

BOOL InitTray(HINSTANCE & hInst, HWND & hWnd, NOTIFYICONDATAW & nid) {
	// NotifyIconData Icon Initialization
	TrayIconInit();
	UpdateTrayIconNumber(CLR_DISABLED, L"--", 0);
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.hIcon = _hicon;
	nid.uCallbackMessage = WM_USER_SHELLICON;
	std::wstring tool_tip = _ts(L"Initializing...");
	wmemcpy_s(nid.szTip, 128, tool_tip.c_str(), tool_tip.length());
	return Shell_NotifyIconW(NIM_ADD, &nid);
}

BOOL UpdateTray(HWND hWnd, NOTIFYICONDATAW & nid, AsusDLL & asus_control, int& current_mode) {
	COLORREF clr;
	switch (current_mode) {
	case ID_POPUP_ECO:
		clr = CLR_ECO;
		break;
	case ID_POPUP_BALANCED:
		clr = CLR_BALANCED;
		break;
	case ID_POPUP_TURBO:
		clr = CLR_TURBO;
		break;
	default:
		clr = CLR_WHITE;
		break;
	}
	if (asus_control.current_fan_test_mode == 0x00) {
		clr = CLR_DISABLED;
	}

	std::wstring tool_tip = _ts(L"Currently ") + (asus_control.current_fan_test_mode ? _ts(L"enabled!") : _ts(L"disabled!"));
	tool_tip += _ts(L"\nCPU: ") + _ts(asus_control.current_cpu_thermal) + _ts(L"°C | GPU: ") + _ts(asus_control.current_gpu_thermal) + _ts(L"°C\n")
		+ _ts(L"Fan: ") + _ts(asus_control.get_fan_speed()) + _ts(L"RPM (") + _ts(asus_control.current_fan_percent) + _ts(L"%)");
	ZeroMemory(&nid.szTip, sizeof(nid.szTip)); // clear the string
	wmemcpy_s(nid.szTip, 128, tool_tip.c_str(), tool_tip.length());

	if (!UpdateTrayIconNumber(clr, NULL, asus_control.get_thermal())) {
		_dErr(L"UpdateTrayIconNumber failed!");
	}
	nid.hIcon = _hicon;
	return Shell_NotifyIconW(NIM_MODIFY, &nid);
}