#include "get_data.h"

void AsusDLL::set_fan_test_mode()
{
}

void AsusDLL::set_fan_pwm_duty()
{
}

AsusDLL::AsusDLL() {
	_dInfo(L"Constructor called");
	if (asus_dll != NULL) {
		_dInfo(L"DLL loaded!");
	}
	else {
		_dErr(L"Failed, didn't load DLL!");
		init_status = false;
		return;
	}
	typedef void (*Cons)();
	Cons initialize = (Cons)GetProcAddress(asus_dll, "InitializeWinIo");
	initialize();
	init_status = true;
	_dInfo(L"Initialized!");
}

AsusDLL::~AsusDLL() {
	_dInfo(L"Destructor called");
	if (asus_dll != NULL) {
		_dInfo(L"DLL loaded!");
	}
	else {
		_dErr(L"Failed, didn't load DLL!");
		init_status = false;
		return;
	}
	typedef void (*Dest)();
	Dest shutdown = (Dest)GetProcAddress(asus_dll, "ShutdownWinIo");
	shutdown();
	init_status = false;
	_dInfo(L"Killed!");
}

int AsusDLL::get_fan_count()
{
	return 0;
}

void AsusDLL::set_fan_idx()
{
}

void AsusDLL::set_fan()
{
}

int AsusDLL::get_fan_speed_idx()
{
	return 0;
}

std::vector<int> AsusDLL::get_fan_speed()
{
	return std::vector<int>();
}

ULONG AsusDLL::get_thermal_cpu()
{
	return 0;
}