// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#pragma once
#include "utils.h"
#include <fstream>
#include <filesystem>

BOOL write_settings(inipp::Ini<wchar_t>& settings);
BOOL read_settings(inipp::Ini<wchar_t>& settings);

const std::wstring CURRENT_SETTINGS_VERSION = L"2";