#pragma once
#include <windows.h>
#include "utils.h"
#include "vector"
#include <string>

using byte = unsigned char;

class AsusDLL {
private:
	HMODULE asus_dll = LoadLibraryW(L"AsusWinIO64.dll");
	
	bool set_fan_pwm_duty(byte value);
	bool failed(const std::wstring reason, bool shown);
	bool success(const std::wstring reason, bool shown);
	int fan_count = -1;

public:
	AsusDLL();
	~AsusDLL();
	bool init_status = false;
	int get_fan_count();
	bool set_fan_speed_idx(byte value, byte fanIdx);
	bool set_fan_speed_idx(int percent, byte fanIdx);
	bool set_fan_speed(byte value);
	bool set_fan_speed(int percent);
	bool set_fan_test_mode(char mode);
	int get_fan_speed_idx(byte fanIdx);
	std::vector<int> get_fan_speed();
	ULONG get_thermal_cpu();
};
