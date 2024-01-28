#pragma once

#include "controller.h"
#include "resource.h"

BOOL UpdateTray(HWND hWnd, NOTIFYICONDATAW& nid, AsusDLL& asus_control, int& current_mode);
BOOL TrayIconInit();
BOOL InitTray(HINSTANCE& hInst, HWND& hWnd, NOTIFYICONDATAW& nid);