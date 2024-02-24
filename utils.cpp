// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#include "utils.h"

#ifdef _DEBUG
void _dInfo(const wchar_t str[]) {
	_dInfo(std::wstring(str));
}
void _dInfo(const std::wstring str) {
	OutputDebugString((_time_str() + L" [Info] " + str + L"\n").c_str());
}
void _dErr(const wchar_t str[]) {
	_dErr(std::wstring(str));
}
void _dErr(const std::wstring str) {
	OutputDebugString((_time_str() + L" [ERROR] " + str + L"\n").c_str());
}
#else
void _dInfo(const wchar_t str[]) {}
void _dInfo(const std::wstring str) {}
void _dErr(const wchar_t str[]) {}
void _dErr(const std::wstring str) {}
#endif

std::wstring _ts(const int& value)
{
	return std::to_wstring(value);
}

std::wstring _ts(const wchar_t str[])
{
	std::wstring new_str(str);
	return new_str;
}

std::wstring _ts(const std::vector<int>& vt)
{
	std::wstring new_str;
	for (int idx = 0; idx < vt.size(); ++idx) {
		new_str += std::to_wstring(vt[idx]);
		if (idx != vt.size() - 1) new_str += L" | ";
	}
	return new_str;
}

ULONGLONG convert_to_ull(SYSTEMTIME& st)
{
	ULARGE_INTEGER ul = { 0 };
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;
	// convert ul to milliseconds
	ul.QuadPart /= 10000;
	return ul.QuadPart;
}

std::wstring _time_str(SYSTEMTIME st) {
	if (st.wYear == 0) {
		// If no time has been transmitted, we use the current time
		GetSystemTime(&st);
	}

	wchar_t buffer[24];
	swprintf(buffer, 24, L"%04d-%02d-%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return std::wstring(buffer);
}