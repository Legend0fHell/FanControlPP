#pragma once

#include "utils.h"
std::vector<std::pair<ULONG, ULONG>> convert_fan_curve(std::wstring str);
ULONG calc_fan_percent(ULONG temperature);
