#include "settings.h"

inipp::Ini<wchar_t> current_settings_data;

BOOL write_settings(inipp::Ini<wchar_t>& settings) {
	// create wofstream settings_file
	std::wofstream settings_file;

	// check if settings file can be opened
	settings_file.open(L"settings.ini");

	if (!settings_file.is_open())
	{
		MessageBox(NULL, L"Failed to create configuration file!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	current_settings_data = settings;

	// write settings file
	settings.generate(settings_file);
	settings_file.flush();
	settings_file.close();
	_dInfo(_ts(L"[Settings] Settings file written successfully!"));

	return TRUE;
}
BOOL create_default_settings() {
	inipp::Ini<wchar_t> settings;

	// default settings
	settings.sections[L"General"][L"UpdateInterval"] = _ts(2000);
	settings.sections[L"General"][L"TrayIconSize"] = _ts(64);
	settings.sections[L"General"][L"TrayIconFont"] = _ts(L"Arial");
	settings.sections[L"General"][L"CurrentMode"] = _ts(ID_POPUP_BALANCED);
	settings.sections[L"General"][L"Startup"] = _ts(0);

	settings.sections[L"Curves"][L"FanEco"] = _ts(L"55c:0%;60c:30%;70c:50%;77c:70%;100c:80%");
	settings.sections[L"Curves"][L"FanBalanced"] = _ts(L"45c:0%;60c:35%;65c:40%;70c:50%;75c:55%;80c:70%;85c:80%;90c:90%");
	settings.sections[L"Curves"][L"FanTurbo"] = _ts(L"30c:0%;45c:35%;65c:45%;70c:50%;74c:55%;77c:61%;80c:75%;83c:90%;88c:100%");

	return write_settings(settings);
}

BOOL read_settings(inipp::Ini<wchar_t>& settings)
{
	// if read settings before, return
	if (current_settings_data.sections.size() > 0) {
		settings = current_settings_data;
		return TRUE;
	}

	// check if settings file exists, if not create default settings
	if (!std::filesystem::exists(L"settings.ini")) {
		_dInfo(_ts(L"[Settings] Settings file not found, creating default settings..."));
		if (!create_default_settings()) {
			return FALSE;
		}
	}
	// create wifstream settings_file
	std::wifstream settings_file;

	// check if settings file can be opened
	settings_file.open(L"settings.ini");

	if (!settings_file.is_open())
	{
		MessageBox(NULL, L"Failed to open configuration file!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	// read settings file
	settings.parse(settings_file);
	current_settings_data = settings;
	_dInfo(_ts(L"[Settings] Settings file read successfully!"));
	settings_file.close();

	return TRUE;
}