// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
// FanControl++.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "FanControl++.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

AsusDLL asus_control;
inipp::Ini<wchar_t> settings;

NOTIFYICONDATAW nid = {};
bool toggle_change_fan_speed = true;
int current_mode = ID_POPUP_BALANCED;
int update_interval = 2000;
bool startup = 0;

// dirty trick
bool thread_term = false;
bool is_about_opened = false;
HWND current_settings_hwnd = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT CALLBACK	About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LONG_PTR lpdata);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FANCONTROL));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	// check for single instance
	HANDLE hMutex = CreateMutexW(NULL, FALSE, L"FanControl++-{10547806-8CBC-4BFC-86D2-4A26475BFB95}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || GetLastError() == ERROR_ACCESS_DENIED) {
		MessageBox(NULL, L"FanControl++ is already running!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return NULL;
	}

	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd) {
		MessageBox(NULL, L"Window creation failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	_dInfo(L"Window initialized!");

	return hWnd;
}

static BOOL SysRegisterRestart(bool is_register) {
	HRESULT status;

	if (is_register)
	{
		status = RegisterApplicationRestart(L"-minimized", RESTART_NO_CRASH);
	}
	else
	{
		status = UnregisterApplicationRestart();
	}

	return SUCCEEDED(status);
}

static BOOL ChangeStartupBehavior(BOOL enable) {
	HKEY hKey;
	LONG lResult;

	lResult = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
	if (lResult != ERROR_SUCCESS) {
		_dErr(_ts(L"[Startup] Cannot open registry key!"));
		MessageBox(NULL, L"Cannot open registry key!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	if (enable) {
		wchar_t szPath[MAX_PATH];
		GetModuleFileNameW(NULL, szPath, MAX_PATH);
		std::wstring tmp(szPath);
		tmp = L"\"" + tmp + L"\" -minimized";

		wcscpy_s(szPath, tmp.c_str());
		lResult = RegSetValueExW(hKey, L"FanControl++", 0, REG_SZ, (LPBYTE)(szPath), sizeof(szPath));

		if (lResult != ERROR_SUCCESS) {
			_dErr(_ts(L"[Startup] Cannot set registry key!"));
			MessageBox(NULL, L"Cannot set registry key!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			RegCloseKey(hKey);
			return FALSE;
		}

		SysRegisterRestart(true);
	}
	else {
		lResult = RegDeleteValueW(hKey, L"FanControl++");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			_dErr(_ts(L"[Startup] Cannot delete registry key!"));
			MessageBox(NULL, L"Cannot delete registry key!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			RegCloseKey(hKey);
			return FALSE;
		}

		SysRegisterRestart(false);
	}

	_dInfo(_ts(L"[Startup] Startup behavior changed to ") + _ts(enable));
	RegCloseKey(hKey);
	return TRUE;
}

static BOOL Cleanup(HWND& hWnd, NOTIFYICONDATAW& nid) {
	return Shell_NotifyIconW(NIM_DELETE, &nid);
}

static BOOL MainThread(HWND hWnd) noexcept(false) {
	ULONGLONG last_update_thread = 0;
	while (!thread_term) {
		if (asus_control.error_occured) {
			_dErr(_ts(L"[Thread] Error occured!"));
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}

		int temp = asus_control.get_thermal();
		update_average_temperature(temp);

		if (toggle_change_fan_speed) {
			int perc = calc_fan_percent(current_mode);
			if (perc < 0) _dErr(_ts(L"[Thread] Invalid fan percent!"));
			else asus_control.set_fan_speed(perc);
		}

		_dInfo(_ts(L"[Thread] Temp: ") + _ts(temp) + _ts(L" | Percent: ") + _ts(trunc(asus_control.current_fan_percent)) + _ts(L" | Mode: ") + _ts(current_mode - ID_POPUP_ECO));

		UpdateTray(hWnd, nid, asus_control, current_mode);

		if (thread_term) break;

		if (current_settings_hwnd) {
			std::wstring tool_tip = _ts(L"CPU: ") + _ts(asus_control.current_cpu_thermal) + _ts(L"°C | GPU: ") + _ts(asus_control.current_gpu_thermal) + _ts(L"°C\n")
				+ _ts(L"Fan: ") + _ts(asus_control.get_fan_speed()) + _ts(L"RPM (") + _ts(asus_control.current_fan_percent) + _ts(L"%)");
			SetWindowTextW(GetDlgItem(current_settings_hwnd, IDC_STATIC_INFO), tool_tip.c_str());
		}

		SYSTEMTIME st;
		GetSystemTime(&st);
		ULONGLONG time_taken = convert_to_ull(st) - last_update_thread;
		if (time_taken < update_interval) {
			Sleep(update_interval - time_taken); // sleep until next update
		}
	}

	return Cleanup(hWnd, nid);
}

static void ShowAboutMessage(HWND hWnd) {
	if (is_about_opened) return;
	is_about_opened = true;
	TASKDIALOGCONFIG tdc = { 0 };
	LPCWSTR str_title = L"About";
	WCHAR str_content[512];
	std::wstring tmp_str;
	std::wstring version = CURRENT_VERSION;
#ifdef _DEBUG
	version += L" (Debug), ";
#else
	version += L" (Release), ";
#endif
	version += (sizeof(void*) == 8 ? L"x64" : L"x86");
	version += L" Unicode.";
	tmp_str = L"A lightweight fan controller for ASUS laptops.\r\n"
		+ _ts(L"Version ") + version + L"\r\n\r\n"
		+ _ts(L"(c) 2024 Phạm Nhật Quang (Legend0fHell).\r\nAll Rights Reserved.\r\n\r\n")
		+ _ts(L"<a href=\"https://github.com/Legend0fHell\">github.com/Legend0fHell</a>\r\n");

	wcscpy_s(str_content, tmp_str.c_str());
	tdc.cbSize = sizeof(tdc);
	tdc.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_EXPAND_FOOTER_AREA | TDF_SIZE_TO_CONTENT;
	tdc.hwndParent = hWnd;
	tdc.hInstance = hInst;
	tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
	tdc.pszFooterIcon = TD_INFORMATION_ICON;
	tdc.pszWindowTitle = str_title;
	tdc.pszMainInstruction = (LPWSTR)IDS_APP_TITLE;
	tdc.pszContent = str_content;
	tdc.pfCallback = &About;
	tdc.lpCallbackData = MAKELONG(0, TRUE); // on top
	tdc.pszMainIcon = MAKEINTRESOURCE(IDI_FANCONTROL);
	tdc.pszFooter = L"FanControl++ is free software, and licensed under the <a href=\"https://www.gnu.org/licenses/gpl-3.0.en.html\">GNU General Public License 3</a>.\r\n"\
		L"Included AsusWinIO64.dll is licensed by (c) ASUSTek COMPUTER INC. and used in compliance with it's EULA.\r\n";

	TaskDialogIndirect(&tdc, NULL, NULL, NULL);
	is_about_opened = false;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT lpClickPoint;
	if (asus_control.error_occured) {
		DestroyWindow(hWnd);
		return -1;
	}
	if (message == WM_TASKBAR_CREATE) {			// Taskbar has been recreated (Explorer crashed?)
		// Display tray icon
		if (!InitTray(hInst, hWnd, nid)) {
			MessageBox(NULL, L"Systray Icon Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			DestroyWindow(hWnd);
			return -1;
		}
	}
	switch (message)
	{
	case WM_DESTROY:
		Cleanup(hWnd, nid);	// Remove Tray Item
		PostQuitMessage(0);							// Quit
		break;
	case WM_USER_SHELLICON:			// sys tray icon Messages
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:		// Click on sys tray icon
		{
			HMENU hMenu, hSubMenu;
			GetCursorPos(&lpClickPoint);

			// Load menu resource
			hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDC_FANCONTROL));
			if (!hMenu)
				return -1;	// !0, message not successful?

			// Select the first submenu
			hSubMenu = GetSubMenu(hMenu, 0);
			if (!hSubMenu) {
				DestroyMenu(hMenu);        // Be sure to Destroy Menu Before Returning
				return -1;
			}

			// Set Enabled State
			CheckMenuItem(hSubMenu, ID_POPUP_ENABLE, MF_BYCOMMAND | (toggle_change_fan_speed ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(hSubMenu, ID_POPUP_STARTUP, MF_BYCOMMAND | (startup ? MF_CHECKED : MF_UNCHECKED));

			CheckMenuItem(hSubMenu, ID_POPUP_ECO, MF_BYCOMMAND | (current_mode == ID_POPUP_ECO ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(hSubMenu, ID_POPUP_BALANCED, MF_BYCOMMAND | (current_mode == ID_POPUP_BALANCED ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(hSubMenu, ID_POPUP_TURBO, MF_BYCOMMAND | (current_mode == ID_POPUP_TURBO ? MF_CHECKED : MF_UNCHECKED));

			// Display menu
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			SendMessage(hWnd, WM_NULL, 0, 0);

			// Kill off objects we're done with
			DestroyMenu(hMenu);
		}
		break;
		}
		break;
	case WM_CLOSE:
		Cleanup(hWnd, nid);	// Remove Tray Item
		DestroyWindow(hWnd);	// Destroy Window
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_POPUP_EXIT:
			Cleanup(hWnd, nid);			// Remove Tray Item
			DestroyWindow(hWnd);		// Destroy Window
			break;
		case ID_POPUP_ABOUT:			// Open about box
			ShowAboutMessage(hWnd);
			break;
		case ID_POPUP_SETTINGS:
			if (current_settings_hwnd == 0) DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGSBOX), hWnd, Settings);
			else {
				SetForegroundWindow(current_settings_hwnd); // focus on the window
			}
			break;
		case ID_POPUP_ENABLE:			// Toggle Enable
			toggle_change_fan_speed = !toggle_change_fan_speed;
			settings.sections[L"General"][L"Enable"] = _ts(toggle_change_fan_speed);
			write_settings(settings);
			asus_control.set_fan_test_mode(toggle_change_fan_speed ? 0x01 : 0x00);
			break;
		case ID_POPUP_STARTUP:			// Toggle Startup
			startup = !startup;
			if (!ChangeStartupBehavior(startup)) startup = !startup; // revert if failed
			settings.sections[L"General"][L"Startup"] = _ts(startup);
			write_settings(settings);
			break;
		case ID_POPUP_ECO:
		case ID_POPUP_BALANCED:
		case ID_POPUP_TURBO:
			current_mode = LOWORD(wParam);
			settings.sections[L"General"][L"CurrentMode"] = _ts(current_mode);
			write_settings(settings);
			UpdateTray(hWnd, nid, asus_control, current_mode);
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;		// Return 0 = Message successfully proccessed
}

// Message handler for about box.
HRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LONG_PTR lpdata)
{
	if (message == TDN_HYPERLINK_CLICKED) {
		ShellExecuteW(NULL, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
	}
	return S_OK;
}

// Message handler for settings box.
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		current_settings_hwnd = hDlg;
		read_settings(settings);
		std::wstring tmp_eco, tmp_balanced, tmp_turbo;
		inipp::extract(settings.sections[L"Curves"][L"FanTurbo"], tmp_turbo);
		inipp::extract(settings.sections[L"Curves"][L"FanBalanced"], tmp_balanced);
		inipp::extract(settings.sections[L"Curves"][L"FanEco"], tmp_eco);

		SetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, update_interval, FALSE);

		SetDlgItemTextW(hDlg, IDC_EDIT_ECO, tmp_eco.c_str());
		SetDlgItemTextW(hDlg, IDC_EDIT_BALANCED, tmp_balanced.c_str());
		SetDlgItemTextW(hDlg, IDC_EDIT_TURBO, tmp_turbo.c_str());

		CheckDlgButton(hDlg, IDC_CHECK_ENABLE, toggle_change_fan_speed);
		CheckDlgButton(hDlg, IDC_CHECK_STARTUP, startup);

		SendMessageW(GetDlgItem(hDlg, IDC_COMBO_MODE), CB_INSERTSTRING, -1, (LPARAM)L"Eco");
		SendMessageW(GetDlgItem(hDlg, IDC_COMBO_MODE), CB_INSERTSTRING, -1, (LPARAM)L"Balanced");
		SendMessageW(GetDlgItem(hDlg, IDC_COMBO_MODE), CB_INSERTSTRING, -1, (LPARAM)L"Turbo");
		SendMessageW(GetDlgItem(hDlg, IDC_COMBO_MODE), CB_SETCURSEL, static_cast<WPARAM>(current_mode - ID_POPUP_ECO), 0);

		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_OK:
		{
			// read settings
			read_settings(settings);

			// update interval
			int tmp_update_interval = GetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, NULL, FALSE);
			if (tmp_update_interval < 500 || tmp_update_interval > 30000) {
				MessageBox(NULL, L"Update interval not in the range of [500;30000]!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
				break;
			}
			update_interval = tmp_update_interval;
			settings.sections[L"General"][L"UpdateInterval"] = _ts(update_interval);

			// update current mode
			current_mode = SendMessageW(GetDlgItem(hDlg, IDC_COMBO_MODE), CB_GETCURSEL, 0, 0) + ID_POPUP_ECO;
			settings.sections[L"General"][L"CurrentMode"] = _ts(current_mode);

			// update fan curves
			wchar_t tmp_eco[256], tmp_balanced[256], tmp_turbo[256];
			GetDlgItemTextW(hDlg, IDC_EDIT_ECO, tmp_eco, 256);
			GetDlgItemTextW(hDlg, IDC_EDIT_BALANCED, tmp_balanced, 256);
			GetDlgItemTextW(hDlg, IDC_EDIT_TURBO, tmp_turbo, 256);

			if (validator(tmp_eco) == false || validator(tmp_balanced) == false || validator(tmp_turbo) == false) {
				MessageBox(NULL, L"Invalid fan curve!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
				break;
			}

			settings.sections[L"Curves"][L"FanEco"] = _ts(tmp_eco);
			settings.sections[L"Curves"][L"FanBalanced"] = _ts(tmp_balanced);
			settings.sections[L"Curves"][L"FanTurbo"] = _ts(tmp_turbo);

			bool tmp_enable = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE);
			bool tmp_startup = IsDlgButtonChecked(hDlg, IDC_CHECK_STARTUP);

			// close dialog
			EndDialog(hDlg, LOWORD(wParam));

			if (tmp_startup != startup) {
				startup = tmp_startup;
				if (!ChangeStartupBehavior(startup)) startup = !startup; // revert if failed
				settings.sections[L"General"][L"Startup"] = _ts(startup);
			}

			if (tmp_enable != toggle_change_fan_speed) {
				toggle_change_fan_speed = tmp_enable;
				asus_control.set_fan_test_mode(toggle_change_fan_speed ? 0x01 : 0x00);
				settings.sections[L"General"][L"Enable"] = _ts(toggle_change_fan_speed);
			}

			// write settings
			write_settings(settings);

			// update tray icon
			UpdateTray(GetParent(hDlg), nid, asus_control, current_mode);

			current_settings_hwnd = 0;
			return (INT_PTR)TRUE;
		}
		case IDC_BUTTON_CANCEL:
		case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			current_settings_hwnd = 0;
			return (INT_PTR)TRUE;
		}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// before doing anything, set the working directory to the executable's directory
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::wstring tmp(path);
	tmp = tmp.substr(0, tmp.find_last_of(L"\\/"));
	SetCurrentDirectoryW(tmp.c_str());
	_dInfo(tmp);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_FANCONTROL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	HWND hWnd = InitInstance(hInstance, nCmdShow);
	if (hWnd == NULL)
	{
		return FALSE;
	}

	// Load settings
	read_settings(settings);
	inipp::extract(settings.sections[L"General"][L"UpdateInterval"], update_interval);
	inipp::extract(settings.sections[L"General"][L"CurrentMode"], current_mode);
	int tmp_re = 0;
	inipp::extract(settings.sections[L"General"][L"Startup"], tmp_re);
	startup = tmp_re;
	ChangeStartupBehavior(startup);

	inipp::extract(settings.sections[L"General"][L"Enable"], tmp_re);
	toggle_change_fan_speed = tmp_re;
	asus_control.set_fan_test_mode(tmp_re ? 0x01 : 0x00);

	// Display tray icon
	if (!InitTray(hInst, hWnd, nid)) {
		MessageBox(NULL, L"Systray Icon Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		DestroyWindow(hWnd);
		return 0;
	}

	if (asus_control.error_occured) {
		DestroyWindow(hWnd);
		return 0;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FANCONTROL));

	MSG msg;

	std::thread main_thread(MainThread, hWnd);

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Cleanup(hWnd, nid);

	// kinda dirty trick, investigate later
	thread_term = true;
	main_thread.join();

	return (int)msg.wParam;
}