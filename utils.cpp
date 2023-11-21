#include "utils.h"

void _dInfo(const wchar_t str[]) {
	std::wstring new_str(str);
	OutputDebugString((L"[Info] " + new_str + L"\n").c_str());
}

void _dInfo(const std::wstring str) {
	OutputDebugString((L"[Info] " + str + L"\n").c_str());
}

void _dErr(const wchar_t str[]) {
	std::wstring new_str(str);
	OutputDebugString((L"[ERROR] " + new_str + L"\n").c_str());
}

void _dErr(const std::wstring str)
{
	OutputDebugString((L"[ERROR] " + str + L"\n").c_str());
}

std::wstring _ts(const int value)
{
	return std::to_wstring(value);
}

std::wstring _ts(const wchar_t str[])
{
	std::wstring new_str(str);
	return new_str;
}

