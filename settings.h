#pragma once
#include "utils.h"
#include <fstream>
#include <filesystem>

BOOL write_settings(inipp::Ini<wchar_t>& settings);
BOOL read_settings(inipp::Ini<wchar_t>& settings);
