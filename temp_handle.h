// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#pragma once

#include "utils.h"
#include "settings.h"
#include <queue>

float calc_fan_percent(int mode);
void update_average_temperature(ULONG temperature, bool smooth_temp);
bool validator(std::wstring& str);
bool validator(wchar_t* str);
