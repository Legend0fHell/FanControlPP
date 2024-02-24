// FanControl++
// (c) 2024 Pham Nhat Quang (Legend0fHell)
#include "temp_handle.h"

std::vector<std::pair<ULONG, ULONG>> convert_fan_curve(std::wstring& str)
{
	std::vector<std::pair<ULONG, ULONG>> result;
	std::pair<ULONG, ULONG> tmp_pair;
	std::wstring tmp_s = L"";
	for (wchar_t& c : str) {
		if (c == ';') {
			result.push_back(tmp_pair);
		}
		else if (c == 'c') {
			tmp_pair.first = std::stoi(tmp_s);
			tmp_s = L"";
		}
		else if (c == '%') {
			tmp_pair.second = std::stoi(tmp_s);
			tmp_s = L"";
		}
		else if (c != ':') {
			tmp_s.push_back(c);
		}
	}
	result.push_back(tmp_pair);
	return result;
}

bool validator(std::wstring& str)
{
	if (str.size() == 0) return false;				// empty string
	if (str[0] < '0' || str[0] > '9') return false;	// first char is not a number
	if (str.back() != '%') return false;				// last char is not '%'

	// check if all chars are valid
	for (wchar_t& c : str) {
		if ((c >= '0' && c <= '9') || (c == ';') || (c == ':') || (c == '%') || (c == 'c')) continue;
		else return false;
	}

	//	check if all chars are in correct order
	for (size_t i = 1; i < str.size(); i++) {
		if (str[i] == 'c') {
			if (str[i - 1] < '0' || str[i - 1] > '9') return false;
		}
		if (str[i] == '%') {
			if (str[i - 1] < '0' || str[i - 1] > '9') return false;
		}
		if (str[i] == ':') {
			if (str[i - 1] != 'c') return false;
		}
		if (str[i] == ';') {
			if (str[i - 1] != '%') return false;
		}
	}

	// check if all numbers are valid
	std::vector<std::pair<ULONG, ULONG>> fan_curve = convert_fan_curve(str);
	if (fan_curve.size() == 0) return false;
	for (std::pair<ULONG, ULONG>& p : fan_curve) {
		if (p.first > 110UL) return false;
		if (p.second > 100UL) return false;
	}
	return true;
}

bool validator(wchar_t* str)
{
	std::wstring tmp_str(str);
	return validator(tmp_str);
}

std::queue<ULONG> temp_history;
std::queue<ULONGLONG> temp_update_history;
ULONG total_temp = 0;

void update_average_temperature(ULONG temperature) {
	SYSTEMTIME st;
	GetSystemTime(&st);

	ULONGLONG current_time = convert_to_ull(st);

	// only keep temperature in the last 8s
	while (!temp_history.empty() && current_time - temp_update_history.front() > 8000) {
		total_temp -= temp_history.front();
		temp_history.pop();
		temp_update_history.pop();
	}
	total_temp += temperature;
	temp_history.push(temperature);
	temp_update_history.push(current_time);
}

float get_average_temperature() {
	if (temp_history.empty()) return 0;
	return (1.0f * total_temp) / temp_history.size();
}

float calc_fan_percent(int mode)
{
	float temperature = get_average_temperature();
	inipp::Ini<wchar_t> settings;
	read_settings(settings);
	std::wstring fan_str;
	switch (mode) {
	case ID_POPUP_ECO:
		inipp::extract(settings.sections[L"Curves"][L"FanEco"], fan_str);
		break;
	case ID_POPUP_BALANCED:
		inipp::extract(settings.sections[L"Curves"][L"FanBalanced"], fan_str);
		break;
	case ID_POPUP_TURBO:
		inipp::extract(settings.sections[L"Curves"][L"FanTurbo"], fan_str);
		break;
	default:
		MessageBox(NULL, L"Invalid operation mode!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		break;
	}
	if (validator(fan_str) == false) {
		MessageBox(NULL, L"Invalid fan curve!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return -1;
	}
	std::vector<std::pair<ULONG, ULONG>> fan_curve = convert_fan_curve(fan_str);

	settings.clear();
	if (temperature <= fan_curve[0].first) return fan_curve[0].second;
	if (temperature >= fan_curve.back().first) return fan_curve.back().second;

	// CP time
	ULONG lft_idx = 0;
	ULONG l = 0, r = int(fan_curve.size()) - 1;
	while (l <= r) {
		ULONG mid = (l + r) / 2;
		if (temperature >= fan_curve[mid].first) {
			lft_idx = mid;
			l = mid + 1;
		}
		else r = mid - 1;
	}
	ULONG rgt_idx = lft_idx + 1;
	ULONG percent_gap = fan_curve[rgt_idx].second - fan_curve[lft_idx].second;
	ULONG temp_gap = fan_curve[rgt_idx].first - fan_curve[lft_idx].first;
	if (temp_gap == 0UL) return fan_curve[lft_idx].second;
	return fan_curve[lft_idx].second + ((temperature - 1.0f * fan_curve[lft_idx].first) * percent_gap) / (1.0f * temp_gap);
}