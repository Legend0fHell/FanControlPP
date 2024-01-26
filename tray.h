#pragma once

#include "get_data.h"

#define	WM_USER_SHELLICON WM_USER + 1
#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))

BOOL InitTray(HWND& hWnd, NOTIFYICONDATAW& nid);
BOOL UpdateTray(HWND& hWnd, NOTIFYICONDATAW& nid, AsusDLL& asus_control);
BOOL TrayIconInit();