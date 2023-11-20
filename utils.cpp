#include "utils.h"

void _dInfo(const wchar_t str[]) {
	std::wstring new_str(str);
	new_str = L"[Info] " + new_str + L"\n";
	OutputDebugString(new_str.c_str());
}

void _dErr(const wchar_t str[]) {
	std::wstring new_str(str);
	new_str = L"[ERROR] " + new_str + L"\n";
	OutputDebugString(new_str.c_str());
}