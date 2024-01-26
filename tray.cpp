#include "tray.h"


HDC _hdc, _hdc_mask;
HBITMAP _hbitmap, _hbitmap_mask;
HFONT _hfont;
HICON _hicon;
RECT _icon_size;

BOOL InitTray(HWND& hWnd, NOTIFYICONDATAW& nid) {
    // NotifyIconData Icon Initialization
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = (HICON)LoadImage(NULL, L"small.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED | LR_LOADTRANSPARENT);
	std::wstring tool_tip = _ts(L"Initializing...");
	wmemcpy_s(nid.szTip, 128, tool_tip.c_str(), tool_tip.length());
    TrayIconInit();

    //nid ucallbackmessage
    nid.uCallbackMessage = WM_USER_SHELLICON;


	return Shell_NotifyIconW(NIM_ADD, &nid);
}

// get dpi value using dpix and dpiy
LONG _r_dc_getdpivalue(HDC hdc, RECT* rect) {
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
        return _r_dc_getdpivalue(NULL, &taskbar_rect.rc);

    return _r_dc_getdpivalue(NULL, NULL);
}

void FontInit(PLOGFONT logfont) {
    RtlZeroMemory(logfont, sizeof(LOGFONT));
    wcscpy_s(logfont->lfFaceName, L"Digital-7 Mono");
    logfont->lfHeight = -(LONG)((TRAY_ICON_SIZE*5/6) * GetTaskbarDPI())/96;
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

    SetRect(&_icon_size, 0, 0, TRAY_ICON_SIZE, TRAY_ICON_SIZE);
    hdcOri = GetDC(NULL);

    if (!hdcOri) {
        _dErr(L"GetDC failed!");
        return FALSE;
    }

    _hdc = CreateCompatibleDC(hdcOri);
    _hdc_mask = CreateCompatibleDC(hdcOri);

    // Create bitmap
    _hbitmap = CreateAlphaBitmap(_hdc, TRAY_ICON_SIZE, TRAY_ICON_SIZE, &bits);
    _hbitmap_mask = CreateBitmap(TRAY_ICON_SIZE, TRAY_ICON_SIZE, 1, 1, NULL);
    ReleaseDC(NULL, hdcOri);
    return TRUE;
}

// draw background on hdc
void DrawBackground(HDC &hdc, COLORREF bg_clr, COLORREF pen_clr, COLORREF brush_clr) {
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

void UpdateText(HDC &hdc, wchar_t* szText, int text_length, PRECT rect, COLORREF clr) {
    COLORREF prev_clr;
    ULONG flags;

    prev_clr = SetTextColor(hdc, clr);

    flags = DT_BOTTOM | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX;

    DrawTextExW(hdc, szText, text_length, rect, flags, NULL);

    SetTextColor(hdc, prev_clr);
}

BOOL UpdateTrayIconNumber(HWND& hWnd, NOTIFYICONDATAW& nid, AsusDLL& asus_control) {
    wchar_t* szText = _wcsdup(_ts(asus_control.get_thermal_cpu()).c_str());
    _dInfo(_ts(L"[UpdateTrayIconNumber] Thermal Updating Icon: ") + _ts(asus_control.get_thermal_cpu()));

    HGDIOBJ prev_bmp, prev_font;
    ICONINFO iconInfo;
    int prev_mode;

    prev_bmp = SelectObject(_hdc, _hbitmap);
    prev_font = SelectObject(_hdc, _hfont);
    prev_mode = SetBkMode(_hdc, TRANSPARENT);

    DrawBackground(_hdc, TRANSPARENT, TRANSPARENT, TRANSPARENT);

    UpdateText(_hdc, szText, lstrlen(szText), &_icon_size, RGB(255, 255 - max(0, (long(asus_control.get_thermal_cpu()) - 60) * 6), 255 - max(0, (long(asus_control.get_thermal_cpu()) - 60) * 6)));

    SetBkMode(_hdc, prev_mode);
    SelectObject(_hdc, prev_font);
    SelectObject(_hdc, prev_bmp);

    // Draw mask
    prev_bmp = SelectObject(_hdc_mask, _hbitmap_mask);
    prev_font = SelectObject(_hdc_mask, _hfont);
    prev_mode = SetBkMode(_hdc_mask, TRANSPARENT);

    DrawBackground(_hdc_mask, RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255));

    UpdateText(_hdc_mask, szText, lstrlen(szText), &_icon_size, RGB(0, 0, 0));

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

BOOL UpdateTray(HWND& hWnd, NOTIFYICONDATAW& nid, AsusDLL& asus_control) {
	std::wstring tool_tip = _ts(L"Temp: ") + _ts(asus_control.get_thermal_cpu()) + _ts(L"°C\n") + _ts(L"Speed: ")
							+ _ts(asus_control.get_fan_speed()) + _ts(L"RPM (") + _ts(asus_control.current_fan_percent) + _ts(L"%)");
	wmemcpy_s(nid.szTip, 128, tool_tip.c_str(), tool_tip.length());
    if (!UpdateTrayIconNumber(hWnd, nid, asus_control)) {
        _dErr(L"UpdateTrayIconNumber failed!");
    }
    nid.hIcon = _hicon;
	BOOL res = Shell_NotifyIconW(NIM_MODIFY, &nid);
	_dInfo(L"Tray updated?");
	return res;
}