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
	int get_fan_count();
	bool set_fan_speed_idx(byte value, byte fanIdx);
	bool init_status = false;
	char current_fan_test_mode = 0x00;
	ULONGLONG last_update_thermal = 0;
	ULONGLONG last_update_fan_speed = 0;
	int fan_count = -1;
	std::vector<int> fan_speed_list;
	int current_combined_thermal = 0;

public:
	AsusDLL();
	~AsusDLL() noexcept(false);
	bool set_fan_speed(int percent);
	bool set_fan_test_mode(char mode);
	int get_fan_speed_idx(byte fanIdx);
	std::vector<int> get_fan_speed();
	float current_fan_percent = 0;
	int current_cpu_thermal = 0;
	int current_gpu_thermal = 0;
	ULONG get_thermal();
	PWRStatus get_power_mode();
};
