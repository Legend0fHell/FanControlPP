// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#pragma once

#include "controller.h"
#include "resource.h"

BOOL UpdateTray(HWND hWnd, NOTIFYICONDATAW& nid, AsusDLL& asus_control, int& current_mode);
BOOL UpdateTrayIconNumber(COLORREF clr, const wchar_t* str, const int num);
BOOL TrayIconInit();
BOOL InitTray(HINSTANCE& hInst, HWND& hWnd, NOTIFYICONDATAW& nid);