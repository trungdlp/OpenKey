#pragma once
#include <windows.h>
#include <string>
#include <vector>

inline bool installUpdate(const std::wstring& directory, const std::wstring& staging) {
    std::vector<std::wstring> names;
    for (const wchar_t* name : {L"OpenKey64.exe", L"OpenKey32.exe"}) {
        const std::wstring destination = directory + L"\\" + name;
        if (GetFileAttributesW(destination.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        DWORD binaryType = 0;
        if (GetFileAttributesW((destination + L".bak").c_str()) != INVALID_FILE_ATTRIBUTES ||
            !GetBinaryTypeW((staging + L"\\" + name).c_str(), &binaryType)) return false;
        names.emplace_back(name);
    }
    if (names.empty()) return false;
    for (size_t pos = 0; pos < names.size(); pos++) {
        const std::wstring destination = directory + L"\\" + names[pos];
        const std::wstring backup = destination + L".bak";
        if (!ReplaceFileW(destination.c_str(), (staging + L"\\" + names[pos]).c_str(), backup.c_str(),
                          REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {
            for (size_t undo = pos + 1; undo > 0; undo--) {
                const std::wstring original = directory + L"\\" + names[undo - 1];
                const std::wstring saved = original + L".bak";
                if (GetFileAttributesW(saved.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
                if (GetFileAttributesW(original.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    MoveFileExW(saved.c_str(), original.c_str(), MOVEFILE_WRITE_THROUGH);
                } else {
                    ReplaceFileW(original.c_str(), saved.c_str(), NULL, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL);
                }
            }
            // Keep any remaining backups if rollback is blocked by another process.
            return false;
        }
    }
    for (const auto& name : names) DeleteFileW((directory + L"\\" + name + L".bak").c_str());
    return true;
}
