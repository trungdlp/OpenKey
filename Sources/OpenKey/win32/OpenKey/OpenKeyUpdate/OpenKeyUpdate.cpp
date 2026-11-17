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

#include "framework.h"
#include "OpenKeyUpdate.h"
#include <Urlmon.h>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include "UpdateInstaller.h"
#pragma comment(lib, "Urlmon.lib")

using namespace std;

INT_PTR CALLBACK MainDialogProcess(HWND, UINT, WPARAM, LPARAM);
void StartUpdate();
HWND hDlg;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_UPDATER), 0, MainDialogProcess, 0);
	ShowWindow(hDlg, SW_SHOWNORMAL);
 
	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK MainDialogProcess(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_OPENKEYUPDATE));
		if (hIcon) {
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
		StartUpdate();
		return (INT_PTR)TRUE;
	}
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

DWORD WINAPI UpdateThreadFunction(LPVOID lpParam) {
	WCHAR currentDir[MAX_PATH];
	const DWORD pathLength = GetModuleFileNameW(NULL, currentDir, MAX_PATH);
	if (!pathLength || pathLength >= MAX_PATH) return 1;
	wchar_t* separator = wcsrchr(currentDir, L'\\');
	if (!separator) return 1;
	*separator = 0;
	const wstring path = wstring(currentDir) + L"\\_OpenKey.tempf";
	HRESULT res = URLDownloadToFile(NULL, L"https://raw.githubusercontent.com/tuyenvm/OpenKey/master/version.json", path.c_str(), 0, NULL);

	wstring data;
	if (res == S_OK) {
		std::wifstream t(path.c_str());
		std::wstringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path.c_str());
		data = buffer.str();
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	std::wsmatch match;
	const std::wregex versionPattern(LR"pattern("latestWinVersion"\s*:\s*\{[^}]*"versionName"\s*:\s*"([A-Za-z0-9._-]+)")pattern");
	if (!std::regex_search(data, match, versionPattern)) {
		MessageBox(hDlg, _T("Dữ liệu phiên bản không hợp lệ. Bản hiện tại được giữ nguyên."), _T("OpenKey Update"), MB_OK);
		ExitProcess(1);
	}
	const wstring versionName = match[1].str();
	WCHAR stagingPath[MAX_PATH];
	if (!GetTempFileNameW(currentDir, L"OKU", 0, stagingPath)) return 1;
	DeleteFileW(stagingPath);
	if (!CreateDirectoryW(stagingPath, NULL)) return 1;
	const wstring staging = stagingPath;
	const wstring archive = staging + L"\\update.zip";
	const wstring extracted = staging + L"\\files";

	//download zip file
	const wstring updateUrl = L"https://github.com/tuyenvm/OpenKey/releases/download/" + versionName + L"/OpenKey" + versionName + L"-Windows.zip";
	res = URLDownloadToFile(NULL, updateUrl.c_str(), archive.c_str(), 0, NULL);

	bool updated = false;
	if (res == S_OK) {
		auto quote = [](const wstring& value) {
			wstring quoted = L"'";
			for (wchar_t ch : value) { quoted += ch; if (ch == L'\'') quoted += ch; }
			return quoted + L"'";
		};
		wstring command = L"powershell.exe -NoP -NonI -Command \"Expand-Archive -LiteralPath " +
			quote(archive) + L" -DestinationPath " + quote(extracted) + L" -Force -ErrorAction Stop\"";
		STARTUPINFOW startup = {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process = {};
		if (CreateProcessW(NULL, &command[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, currentDir, &startup, &process)) {
			DWORD exitCode = 1;
			if (WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0 &&
				GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0) {
				updated = installUpdate(currentDir, extracted);
			}
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
		}
	}
	if (updated) {
		DeleteFileW(archive.c_str());
		for (const wchar_t* file : {L"OpenKey64.exe", L"OpenKey32.exe", L"OpenKeyUpdate.exe"}) {
			DeleteFileW((extracted + L"\\" + file).c_str());
		}
		RemoveDirectoryW(extracted.c_str());
		RemoveDirectoryW(staging.c_str());
		MessageBox(hDlg, _T("Bạn đã cập nhật OpenKey bản mới nhất thành công!"), _T("OpenKey Update"), MB_OK);
	} else {
		MessageBox(hDlg, _T("Cập nhật thất bại. Bản cũ hoặc bản sao .bak được giữ lại; vui lòng đóng OpenKey và thử lại."), _T("OpenKey Update"), MB_OK);
	}
	ExitProcess(updated ? 0 : 1);
	return 0;
}

void StartUpdate() {
	DWORD hThread;
	HANDLE t = CreateThread(
							NULL,                   // default security attributes
							0,                      // use default stack size  
							UpdateThreadFunction,       // thread function name
							0,          // argument to thread function 
							0,                      // use default creation flags 
							&hThread);   // returns the thread identifier 
}
