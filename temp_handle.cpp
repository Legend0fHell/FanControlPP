#include "temp_handle.h"

std::vector<std::pair<ULONG, ULONG>> convert_fan_curve(std::wstring str)
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

ULONG calc_fan_percent(ULONG temperature, int mode)
{
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
	std::vector<std::pair<ULONG, ULONG>> fan_curve = convert_fan_curve(fan_str);

	settings.clear();
	if (fan_curve.size() == 0) return -1;
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
	return fan_curve[lft_idx].second + percent_gap * (temperature - fan_curve[lft_idx].first) / temp_gap;
}