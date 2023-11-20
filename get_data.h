#pragma once
#include <windows.h>
#include "utils.h"
#include "vector"

class AsusDLL {
private:
	HMODULE asus_dll = LoadLibraryW(L"AsusWinIO64.dll");
	void set_fan_test_mode();
	void set_fan_pwm_duty();

public:
	AsusDLL();
	~AsusDLL();
	bool init_status = false;
	int get_fan_count();
	void set_fan_idx();
	void set_fan();
	int get_fan_speed_idx();
	std::vector<int> get_fan_speed();
	ULONG get_thermal_cpu();
};
