#pragma once

#include "utils.h"
#include "settings.h"

using byte = unsigned char;
enum PWRStatus {
	DC, AC, Error
};

class AsusDLL {
private:
	HMODULE asus_dll = LoadLibraryW(L"AsusWinIO64.dll");
	
	bool set_fan_pwm_duty(byte value);
	bool failed(const std::wstring reason, bool shown);
	bool success(const std::wstring reason, bool shown);
	bool init_status = false;
	ULONGLONG last_update_thermal_cpu = 0;
	int fan_count = -1;

public:
	AsusDLL();
	~AsusDLL() noexcept(false);
	int get_fan_count();
	bool set_fan_speed_idx(byte value, byte fanIdx);
	bool set_fan_speed_idx(int percent, byte fanIdx);
	bool set_fan_speed(byte value);
	bool set_fan_speed(int percent);
	bool set_fan_test_mode(char mode);
	int get_fan_speed_idx(byte fanIdx);
	std::vector<int> get_fan_speed();
	int current_fan_percent = 0;
	int current_thermal_cpu = 0;
	int current_thermal_gpu = 0;
	ULONG get_thermal_cpu();
	ULONG get_thermal_gpu();
	PWRStatus get_power_mode();
};
