// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "inipp.h"
#include "resource.h"

void _dInfo(const wchar_t str[]);
void _dInfo(const std::wstring str);
void _dErr(const wchar_t str[]);
void _dErr(const std::wstring str);

std::wstring _ts(const int& value);
std::wstring _ts(const wchar_t str[]);
std::wstring _ts(const std::vector<int>& vt);

ULONGLONG convert_to_ull(SYSTEMTIME& st);
std::wstring _time_str(SYSTEMTIME st = {});

#define	WM_USER_SHELLICON WM_USER + 1
#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))
