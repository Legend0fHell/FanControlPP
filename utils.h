#pragma once
#include <Windows.h>
#include <string>

void _dInfo(const wchar_t str[]);
void _dInfo(const std::wstring str);
void _dErr(const wchar_t str[]);
void _dErr(const std::wstring str);

std::wstring _ts(const int value);
std::wstring _ts(const wchar_t str[]);