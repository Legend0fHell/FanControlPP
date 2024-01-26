#include "get_data.h"

bool AsusDLL::failed(const std::wstring reason = _ts(L"Failed"), bool shown = true) {
	if(shown) _dErr(reason);
	return false;
}

bool AsusDLL::success(const std::wstring reason = _ts(L"Success"), bool shown = true)
{
	if (shown) _dInfo(reason);
	return true;
}

AsusDLL::AsusDLL() {
	_dInfo(L"Constructor called");
	if (asus_dll == NULL) {
		failed(_ts(L"DLL not loaded yet!!"));
		return;
	}
	typedef void (*Cons)();
	Cons initialize = (Cons)GetProcAddress(asus_dll, "InitializeWinIo");
	initialize();
	init_status = true;
	_dInfo(L"Initialized!");
}

AsusDLL::~AsusDLL() noexcept(false) {
	_dInfo(L"Destructor called");
	AsusDLL::set_fan_test_mode(0x00);
	typedef void (*Dest)();
	Dest shutdown = (Dest)GetProcAddress(asus_dll, "ShutdownWinIo");
	shutdown();
	init_status = false;
	_dInfo(L"Killed!");
}

int AsusDLL::get_fan_count()
{
	if (!init_status) {
		_dErr(L"Get fan count failed");
		return -1;
	}
	if (fan_count != -1) return fan_count;
	typedef int (*FanCount)();
	FanCount fan_count = (FanCount)GetProcAddress(asus_dll, "HealthyTable_FanCounts");
	return (int)fan_count();
}

bool AsusDLL::set_fan_test_mode(char mode)
{
	if (!init_status) return failed();
	if (mode == 0x00) current_fan_percent = 0;
	typedef void (*FanTestMode)(char value);
	FanTestMode set_fan_test_mode = (FanTestMode)GetProcAddress(asus_dll, "HealthyTable_SetFanTestMode");
	set_fan_test_mode(mode);
	return success(L"", false);
}

bool AsusDLL::set_fan_pwm_duty(byte value)
{
	if (!init_status) return failed();
	typedef void (*FanPWM)(byte value);
	FanPWM set_fan_pwm = (FanPWM)GetProcAddress(asus_dll, "HealthyTable_SetFanPwmDuty");
	set_fan_pwm(value);
	return success(L"", false);
}

bool AsusDLL::set_fan_speed_idx(byte value, byte fanIdx = 0)
{
	if (!init_status) return failed(_ts(L"Set fan speed #") + _ts(fanIdx) + _ts(L" failed"));
	typedef void (*FanIdx)(byte fanIndex);
	FanIdx set_fan_idx = (FanIdx)GetProcAddress(asus_dll, "HealthyTable_SetFanIndex");
	set_fan_idx(fanIdx);
	AsusDLL::set_fan_test_mode(char(value > 0 ? 0x01 : 0x00));
	AsusDLL::set_fan_pwm_duty(value);
	return success(_ts(L"Set fan speed #") + _ts(fanIdx) + _ts(L" to ") + _ts(value));
}

bool AsusDLL::set_fan_speed_idx(int percent, byte fanIdx = 0)
{
	byte value = (byte)(percent / 100.0f * 255);
	return AsusDLL::set_fan_speed_idx(value, fanIdx);
}

bool AsusDLL::set_fan_speed(byte value)
{
	if (!init_status) return failed();
	int fan_cnt = AsusDLL::get_fan_count();
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		if (!AsusDLL::set_fan_speed_idx(value, fanIdx)) return failed();
		Sleep(200);
	}
	return success();
}

bool AsusDLL::set_fan_speed(int percent)
{
	if (!init_status) return failed();
	byte value = (byte)(percent / 100.0f * 255);

	// wont change if the delta is too small
	if (abs(int(AsusDLL::current_fan_percent - percent)) <= 1) return success();

	// soften the curve e.g. not change the speed too fast
	if (abs(int(AsusDLL::current_fan_percent - percent)) >= 12) {
		if (AsusDLL::current_fan_percent < percent) AsusDLL::current_fan_percent += 12;
		else AsusDLL::current_fan_percent -= 12;
	}

	AsusDLL::current_fan_percent = percent;
	int fan_cnt = AsusDLL::get_fan_count();
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		if (!AsusDLL::set_fan_speed_idx(value, fanIdx)) return failed();
		Sleep(200);
	}
	return success();
}

int AsusDLL::get_fan_speed_idx(byte fanIdx = 0)
{
	if (!init_status) {
		_dErr(_ts(L"Get fan speed #") + _ts(fanIdx) + _ts(L" failed"));
		return -1;
	}
	typedef void (*FanIdx)(byte fanIndex);
	FanIdx set_fan_idx = (FanIdx)GetProcAddress(asus_dll, "HealthyTable_SetFanIndex");
	set_fan_idx(fanIdx);
	typedef int (*FanSpd)();
	FanSpd fan_speed = (FanSpd)GetProcAddress(asus_dll, "HealthyTable_FanRPM");
	return (int)fan_speed();
}

std::vector<int> AsusDLL::get_fan_speed()
{
	int fan_cnt = AsusDLL::get_fan_count();
	std::vector<int> fan_speed_list;
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		int val = AsusDLL::get_fan_speed_idx(fanIdx);
		if (val == -1) break;
		fan_speed_list.push_back(val);
		Sleep(200);
	}
	return fan_speed_list;
}

ULONG AsusDLL::get_thermal_cpu()
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	if(convert_to_ull(st) - last_update_thermal_cpu < UPDATE_INTERVAL) return current_thermal_cpu;

	if (!init_status) {
		_dErr(L"Get thermal CPU failed");
		return -1;
	}

	typedef int (*TherCPU)();
	TherCPU thermal_cpu = (TherCPU)GetProcAddress(asus_dll, "Thermal_Read_Cpu_Temperature");
	current_thermal_cpu = (ULONG)thermal_cpu();
	last_update_thermal_cpu = convert_to_ull(st);

	return current_thermal_cpu;
}

ULONG AsusDLL::get_thermal_gpu()
{
	if (!init_status) {
		_dErr(L"Get thermal GPU failed");
		return -1;
	}
	typedef int (*TherGPU)();
	TherGPU thermal_gpu = (TherGPU)GetProcAddress(asus_dll, "Thermal_Read_GpuTS1L_Temperature");
	return (ULONG)thermal_gpu();
}

PWRStatus AsusDLL::get_power_mode()
{
	if (!init_status) {
		_dErr(L"Get power mode failed");
		return Error;
	}
	typedef byte (*PowerMode)();
	PowerMode power_mode = (PowerMode)GetProcAddress(asus_dll, "Power_Read_ACDCMode");
	return (PWRStatus)power_mode();
}
