// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#include "controller.h"

bool AsusDLL::failed(const std::wstring reason = _ts(L"Failed"), bool shown = true) {
	if (shown) _dErr(reason);
	return false;
}

bool AsusDLL::success(const std::wstring reason = _ts(L"Success"), bool shown = false)
{
	if (shown) _dInfo(reason);
	return true;
}

static bool service_running(std::wstring token)
{
	SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
	if (scm == NULL)
		return false;
	LPCWSTR   lpServiceName = (LPCWSTR)token.data();

	SC_HANDLE hService = OpenService(scm, lpServiceName, GENERIC_READ);
	if (hService == NULL)
	{
		CloseServiceHandle(scm);
		return false;
	}

	SERVICE_STATUS status;
	LPSERVICE_STATUS pstatus = &status;
	if (QueryServiceStatus(hService, pstatus) == 0)
	{
		CloseServiceHandle(hService);
		CloseServiceHandle(scm);
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(scm);
	return (status.dwCurrentState == SERVICE_RUNNING) ? (true) : (false);
}

static bool service_checker() {
	if (service_running(L"ASUSSystemAnalysis")) return TRUE;
	_dErr(_ts(L"[Service Control] Service not running yet!!"));
	bool success = false;

	if (IsUserAnAdmin()) {
		// If user is admin, try to start the service using OpenSCManager

		SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (scm != NULL) {
			SC_HANDLE hService = OpenService(scm, L"ASUSSystemAnalysis", SERVICE_CHANGE_CONFIG | SERVICE_START);
			if (hService != NULL) {
				bool stService = StartService(hService, 0, NULL);
				if (!stService) {
					ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
					_dInfo(L"[Service Control] Changed to Manual");
					stService = StartService(hService, 0, NULL);
					if (stService) {
						MessageBox(NULL,
							_ts(L"ASUSSystemAnalysis Service *was* not running.\nSince you started the program with Administrator rights, the service was started by the program.\nFrom now on, it will automatically be started when you use this program.").c_str(),
							_ts(L"Service notice!").c_str(), MB_ICONINFORMATION | MB_OK);
					}
				}

				success = stService;

				CloseServiceHandle(hService);
			}
			CloseServiceHandle(scm);

		}

		if (!success) {
			MessageBox(NULL, _ts(L"ASUSSystemAnalysis Service is not running!\nPlease start the service manually!").c_str(), _ts(L"Error").c_str(), MB_ICONERROR | MB_OK);
		}
	}
	else {
		MessageBox(NULL, _ts(L"ASUSSystemAnalysis Service is not running!\nPlease start the service first, or run this program again with Administrator rights.").c_str(), _ts(L"Error").c_str(), MB_ICONERROR | MB_OK);
	}
	return success;
}

AsusDLL::AsusDLL() {
	_dInfo(L"Constructor called");
	if (asus_dll == NULL) {
		failed(_ts(L"DLL not loaded yet!!"));
		MessageBox(NULL, _ts(L"Cannot initiate DLL!\nPlease recheck if AsusWinIO64.dll is in the same folder with this executable!").c_str(), _ts(L"Error").c_str(), MB_ICONERROR | MB_OK);
		error_occured = true;
		return;
	}

	if (!service_checker()) {
		_dErr(L"[Service Control] Service failed to start!");
		error_occured = true;
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
	if (!init_status) {
		_dErr(L"Already killed!");
		return;
	}
	AsusDLL::set_fan_test_mode(0x00);
	typedef void (*Dest)();
	Dest shutdown = (Dest)GetProcAddress(asus_dll, "ShutdownWinIo");
	shutdown();
	FreeLibrary(asus_dll);
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
	int fnc = (int)fan_count();
	if (fnc < 1) {
		MessageBox(NULL, _ts(L"Cannot find any fan!").c_str(), _ts(L"Error").c_str(), MB_ICONERROR | MB_OK);
		error_occured = true;
	}
	return fnc;
}

bool AsusDLL::set_fan_test_mode(char mode)
{
	if (!init_status) return failed();
	if (current_fan_test_mode == mode) return success();
	if (mode == 0x00) current_fan_percent = 0;
	int fan_cnt = AsusDLL::get_fan_count();

	typedef void (*FanTestMode)(char value);
	typedef void (*FanIdx)(byte fanIndex);
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		FanIdx set_fan_idx = (FanIdx)GetProcAddress(asus_dll, "HealthyTable_SetFanIndex");
		set_fan_idx(fanIdx);
		FanTestMode set_fan_test_mode = (FanTestMode)GetProcAddress(asus_dll, "HealthyTable_SetFanTestMode");
		set_fan_test_mode(mode);
	}

	current_fan_test_mode = mode;
	return success(_ts(L"[ASUS Service] Fan test mode changed to ") + _ts(mode), true);
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
	if (current_fan_test_mode == 0x00) {
		return success(_ts(L"Fan test mode is not enabled, skip setting fan speed #") + _ts(fanIdx), false);
	}
	typedef void (*FanIdx)(byte fanIndex);
	FanIdx set_fan_idx = (FanIdx)GetProcAddress(asus_dll, "HealthyTable_SetFanIndex");
	set_fan_idx(fanIdx);
	AsusDLL::set_fan_pwm_duty(value);
	return success(_ts(L"Set fan speed #") + _ts(fanIdx) + _ts(L" to ") + _ts(value), false);
}

bool AsusDLL::set_fan_speed(float percent)
{
	if (!init_status) return failed();

	// wont change if the delta is too small
	if (abs(AsusDLL::current_fan_percent - percent) <= 1) return success();

	// set to 0 if the percent is too small
	if (percent < 10) percent = 0;

	AsusDLL::current_fan_percent = percent;

	byte value = max((byte)1, (byte)(AsusDLL::current_fan_percent / 100.0f * 255));

	int fan_cnt = AsusDLL::get_fan_count();
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		if (!AsusDLL::set_fan_speed_idx(value, fanIdx)) return failed();
	}
	return success(_ts(L"Set fan speed to ") + _ts(value), true);
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

static bool update_needed(ULONGLONG& last_update, SYSTEMTIME& st) {
	GetSystemTime(&st);

	inipp::Ini<wchar_t> settings;
	read_settings(settings);
	int update_interval = 2000;
	inipp::extract(settings.sections[L"General"][L"UpdateInterval"], update_interval);
	settings.clear();

	return (convert_to_ull(st) - last_update > update_interval);
}

std::vector<int> AsusDLL::get_fan_speed()
{
	SYSTEMTIME st;
	if (!update_needed(last_update_fan_speed, st)) return fan_speed_list;

	int fan_cnt = AsusDLL::get_fan_count();
	fan_speed_list.clear();
	for (byte fanIdx = 0; fanIdx < fan_cnt; ++fanIdx) {
		int val = AsusDLL::get_fan_speed_idx(fanIdx);
		if (val == -1) break;
		fan_speed_list.push_back(val);
	}

	last_update_fan_speed = convert_to_ull(st);
	return fan_speed_list;
}

ULONG AsusDLL::get_thermal()
{
	SYSTEMTIME st;
	if (!update_needed(last_update_thermal, st)) return current_combined_thermal;

	if (!init_status) {
		_dErr(L"Get thermal failed");
		return -1;
	}

	typedef int (*TherCPU)();
	TherCPU thermal_cpu = (TherCPU)GetProcAddress(asus_dll, "Thermal_Read_Cpu_Temperature");
	typedef int (*TherGPU)();
	TherGPU thermal_gpu = (TherGPU)GetProcAddress(asus_dll, "Thermal_Read_GpuTS1L_Temperature");
	current_cpu_thermal = thermal_cpu();
	current_gpu_thermal = thermal_gpu();
	current_combined_thermal = max(current_cpu_thermal, current_gpu_thermal);
	last_update_thermal = convert_to_ull(st);

	return current_combined_thermal;
}

PWRStatus AsusDLL::get_power_mode()
{
	if (!init_status) {
		_dErr(L"Get power mode failed");
		return Error;
	}
	typedef byte(*PowerMode)();
	PowerMode power_mode = (PowerMode)GetProcAddress(asus_dll, "Power_Read_ACDCMode");
	return (PWRStatus)power_mode();
}