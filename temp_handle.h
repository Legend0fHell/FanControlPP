#pragma once

#include "utils.h"
#include "settings.h"

ULONG calc_fan_percent(ULONG temperature, int mode);
bool validator(std::wstring& str);
bool validator(wchar_t* str);
