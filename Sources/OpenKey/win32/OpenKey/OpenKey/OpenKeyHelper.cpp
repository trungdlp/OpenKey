/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "OpenKeyHelper.h"
#include <stdarg.h>
#include <Urlmon.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "Urlmon.lib")

static BYTE* _regData = 0;

static LPCTSTR sk = TEXT("SOFTWARE\\TuyenMai\\OpenKey");
static HKEY hKey;
static LPCTSTR _runOnStartupKeyPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static TCHAR _executePath[MAX_PATH];
static bool _hasGetPath = false;

static DWORD _cacheProcessId = 0, _tempProcessId = 0;
static HWND _tempWnd;
static TCHAR _exePath[1024] = { 0 };
static LPCTSTR _exeName = _exePath;
static HANDLE _proc;
static string _exeNameUtf8 = "TheOpenKeyProject";
static string _unknownProgram = "UnknownProgram";

int CF_RTF = RegisterClipboardFormat(_T("Rich Text Format"));
int CF_HTML = RegisterClipboardFormat(_T("HTML Format"));
int CF_OPENKEY = RegisterClipboardFormat(_T("OpenKey Format"));

void OpenKeyHelper::openKey() {
	LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, sk, NULL, KEY_ALL_ACCESS, &hKey);
	if (nError == ERROR_FILE_NOT_FOUND) 	{
		nError = RegCreateKeyEx(HKEY_CURRENT_USER, sk, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey, NULL);
	}
	if (nError) {
		LOG(L"result %d\n", nError);
	}
}

void OpenKeyHelper::setRegInt(LPCTSTR key, const int & val) {
	openKey();
	RegSetValueEx(hKey, key, 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
	RegCloseKey(hKey);
}

int OpenKeyHelper::getRegInt(LPCTSTR key, const int & defaultValue) {
	openKey();
	int val = defaultValue;
	DWORD size = sizeof(val);
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, key, 0, 0, (LPBYTE)&val, &size)) {
		val = defaultValue;
	}
	RegCloseKey(hKey);
	return val;
}

void OpenKeyHelper::setRegBinary(LPCTSTR key, const BYTE * pData, const int & size) {
	openKey();
	RegSetValueEx(hKey, key, 0, REG_BINARY, pData, size);
	RegCloseKey(hKey);
}

BYTE * OpenKeyHelper::getRegBinary(LPCTSTR key, DWORD& outSize) {
	openKey();
	if (_regData) {
		delete[] _regData;
		_regData = NULL;
	}
	DWORD size = 0;
	RegQueryValueEx(hKey, key, 0, 0, 0, &size);
	_regData = new BYTE[size];
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, key, 0, 0, _regData, &size)) {
		delete[] _regData;
		_regData = NULL;
		size = 0;
	}
	outSize = size;
	RegCloseKey(hKey);
	return _regData;
}

void OpenKeyHelper::registerRunOnStartup(const int& val) {
	if (val) {
		if (vRunAsAdmin) {
			string path = wideStringToUtf8(getFullPath());
			char buff[MAX_PATH];
			sprintf_s(buff, "schtasks /create /sc onlogon /tn OpenKey /rl highest /tr \"%s\" /f", path.c_str());
			WinExec(buff, SW_HIDE);
		} else {
			RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
			wstring path = getFullPath();
			RegSetValueEx(hKey, _T("OpenKey"), 0, REG_SZ, (byte*)path.c_str(), ((DWORD)path.size() + 1) * sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	} else {
		RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
		RegDeleteValue(hKey, _T("OpenKey"));
		RegCloseKey(hKey);
		WinExec("schtasks /delete  /tn OpenKey /f", SW_HIDE);
	}
}

LPTSTR OpenKeyHelper::getExecutePath() {
	if (!_hasGetPath) {
		HMODULE hModule = GetModuleHandleW(NULL);
		GetModuleFileNameW(hModule, _executePath, MAX_PATH);
		_hasGetPath = true;
	}
	return _executePath;
}

string& OpenKeyHelper::getFrontMostAppExecuteName() {
	_tempWnd = GetForegroundWindow();
	GetWindowThreadProcessId(_tempWnd, &_tempProcessId);
	if (_tempProcessId == _cacheProcessId) {
		return _exeNameUtf8;
	}
	_cacheProcessId = _tempProcessId;
	_proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _tempProcessId);
	GetProcessImageFileName((HMODULE)_proc, _exePath, 1024);
	CloseHandle(_proc);
	
	if (wcscmp(_exePath, _T("")) == 0) {
		return _unknownProgram;
	}
	_exeName = _tcsrchr(_exePath, '\\') + 1;
	if (wcscmp(_exeName, _T("OpenKey64.exe")) == 0 ||
		wcscmp(_exeName, _T("OpenKey32.exe")) == 0 || 
		wcscmp(_exeName, _T("explorer.exe")) == 0) {
		return _exeNameUtf8;
	}
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), &strTo[0], size_needed, NULL, NULL);
	_exeNameUtf8 = strTo;
	//LOG(L"%s\n", utf8ToWideString(_exeNameUtf8).c_str());
	return _exeNameUtf8;
}

string & OpenKeyHelper::getLastAppExecuteName() {
	if (!vUseSmartSwitchKey)
		return getFrontMostAppExecuteName();
	return _exeNameUtf8;
}

wstring OpenKeyHelper::getFullPath() {
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	wstring rs(path);
	return rs;
}

template<typename Char>
static bool readClipboardString(UINT format, std::basic_string<Char>& text) {
	HANDLE data = GetClipboardData(format);
	if (!data) return false;
	const size_t capacity = GlobalSize(data) / sizeof(Char);
	const Char* chars = static_cast<const Char*>(GlobalLock(data));
	if (!chars) return false;
	size_t length = 0;
	while (length < capacity && chars[length]) length++;
	if (length < capacity) text.assign(chars, length);
	GlobalUnlock(data);
	return length < capacity;
}

wstring OpenKeyHelper::getClipboardText(const int& type) {
	if (!OpenClipboard(NULL)) return L"";
	wstring text;
	readClipboardString(type, text);
	CloseClipboard();
	return text;
}

bool OpenKeyHelper::setClipboardText(LPCTSTR data, const int& len, const int& type) {
	if (!data || len <= 0) return false;
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	if (!memory) return false;
	void* destination = GlobalLock(memory);
	if (!destination) { GlobalFree(memory); return false; }
	memcpy(destination, data, len * sizeof(WCHAR));
	GlobalUnlock(memory);
	if (!OpenClipboard(SystemTrayHelper::getWindow())) { GlobalFree(memory); return false; }
	const bool success = EmptyClipboard() && SetClipboardData(type, memory);
	CloseClipboard();
	if (!success) GlobalFree(memory);
	return success;
}

bool OpenKeyHelper::quickConvert(const ConvertOptions& options, HWND owner) {
	if (!OpenClipboard(owner)) return false;
	const DWORD sequence = GetClipboardSequenceNumber();
	const bool hasHTML = IsClipboardFormatAvailable(CF_HTML) != FALSE;
	const bool hasUnicode = IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
	string html;
	wstring unicode;
	const bool valid = (hasHTML || hasUnicode) && (!hasHTML || readClipboardString(CF_HTML, html)) &&
		(!hasUnicode || readClipboardString(CF_UNICODETEXT, unicode));
	CloseClipboard();
	if (!valid) return false;
	if (hasHTML) html = convertUtil(html, options);
	if (hasUnicode) unicode = utf8ToWideString(convertUtil(wideStringToUtf8(unicode), options));

	auto allocate = [](const void* data, size_t bytes) -> HGLOBAL {
		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
		if (!memory) return NULL;
		void* destination = GlobalLock(memory);
		if (!destination) { GlobalFree(memory); return NULL; }
		memcpy(destination, data, bytes);
		GlobalUnlock(memory);
		return memory;
	};
	HGLOBAL htmlMemory = hasHTML ? allocate(html.c_str(), html.size() + 1) : NULL;
	HGLOBAL unicodeMemory = hasUnicode ? allocate(unicode.c_str(), (unicode.size() + 1) * sizeof(wchar_t)) : NULL;
	bool success = false;
	if ((!hasHTML || htmlMemory) && (!hasUnicode || unicodeMemory) && OpenClipboard(owner)) {
		if (GetClipboardSequenceNumber() == sequence && EmptyClipboard()) {
			success = true;
			if (htmlMemory) {
				if (SetClipboardData(CF_HTML, htmlMemory)) htmlMemory = NULL;
				else success = false;
			}
			if (unicodeMemory) {
				if (SetClipboardData(CF_UNICODETEXT, unicodeMemory)) unicodeMemory = NULL;
				else success = false;
			}
		}
		CloseClipboard();
	}
	if (htmlMemory) GlobalFree(htmlMemory);
	if (unicodeMemory) GlobalFree(unicodeMemory);
	return success;
}

DWORD OpenKeyHelper::getVersionNumber() {
	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) {
		return 0;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	UINT dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0) {
		return 0;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0])) {
		return 0;
	}

	LPBYTE lpBuffer = NULL;

	if (VerQueryValue(&data[0], _T("\\"), (VOID FAR * FAR*) & lpBuffer, &dwSize)) {
		if (dwSize) {
			VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			if (verInfo->dwSignature == 0xfeef04bd) {
				return ((verInfo->dwFileVersionMS >> 16) & 0xffff) |
					(((verInfo->dwFileVersionMS >> 0) & 0xffff) << 8) |
					(((verInfo->dwFileVersionLS >> 16) & 0xffff) << 16);
			}
		}
	}

	return 0;
}

wstring OpenKeyHelper::getVersionString() {
	TCHAR versionBuffer[MAX_PATH];
	DWORD ver = getVersionNumber();
	wsprintfW(versionBuffer, _T("%d.%d.%d"), ver & 0xFF, (ver>>8) & 0xFF, (ver >> 16) & 0xFF);
	return wstring(versionBuffer);

	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) { 
		return _T("");
	}
}

wstring OpenKeyHelper::getContentOfUrl(LPCTSTR url){
	WCHAR path[MAX_PATH];
	GetTempPath2(MAX_PATH, path);
	wsprintf(path, TEXT("%s\\_OpenKey.tempf"), path);
	HRESULT res = URLDownloadToFile(NULL, url, path, 0, NULL);
	
	if (res == S_OK) {
		std::wifstream t(path);
		std::wstringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		return buffer.str();
	} else if (res == E_OUTOFMEMORY) {
		
	} else if (res == INET_E_DOWNLOAD_FAILURE) {
		
	} else {
		
	}
	return L"";
}