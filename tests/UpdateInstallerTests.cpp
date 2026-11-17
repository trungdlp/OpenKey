#include "../Sources/OpenKey/win32/OpenKey/OpenKeyUpdate/UpdateInstaller.h"
#include <cassert>
#include <fstream>
#include <iostream>

static void write(const std::wstring& path, const char* contents) {
    std::ofstream file(path.c_str(), std::ios::binary);
    file << contents;
    assert(file.good());
}

static std::string read(const std::wstring& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

int main() {
    wchar_t temp[MAX_PATH], path[MAX_PATH], executable[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp));
    assert(GetTempFileNameW(temp, L"OKT", 0, path));
    assert(DeleteFileW(path));
    assert(CreateDirectoryW(path, NULL));
    const std::wstring directory = path;
    const std::wstring staging = directory + L"\\staging";
    assert(CreateDirectoryW(staging.c_str(), NULL));
    assert(GetModuleFileNameW(NULL, executable, MAX_PATH));
    const auto old64 = directory + L"\\OpenKey64.exe";
    const auto old32 = directory + L"\\OpenKey32.exe";
    assert(!installUpdate(directory, staging));
    write(old64, "old64");
    write(old32, "old32");
    assert(!installUpdate(directory, staging));
    assert(read(old64) == "old64" && read(old32) == "old32");
    write(staging + L"\\OpenKey64.exe", "not an executable");
    assert(!installUpdate(directory, staging));
    assert(read(old64) == "old64");
    assert(CopyFileW(executable, (staging + L"\\OpenKey64.exe").c_str(), FALSE));
    assert(!installUpdate(directory, staging)); // All replacements must validate before the first write.
    assert(read(old64) == "old64");
    assert(CopyFileW(executable, (staging + L"\\OpenKey32.exe").c_str(), FALSE));
    write(old64 + L".bak", "retained backup");
    assert(!installUpdate(directory, staging));
    assert(read(old64 + L".bak") == "retained backup");
    assert(DeleteFileW((old64 + L".bak").c_str()));
    HANDLE locked = CreateFileW(old32.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    assert(locked != INVALID_HANDLE_VALUE);
    assert(!installUpdate(directory, staging));
    CloseHandle(locked);
    assert(read(old64) == "old64" && read(old32) == "old32");
    assert(CopyFileW(executable, (staging + L"\\OpenKey64.exe").c_str(), FALSE));
    assert(CopyFileW(executable, (staging + L"\\OpenKey32.exe").c_str(), FALSE));
    assert(installUpdate(directory, staging));
    assert(read(old64) == read(executable));
    assert(read(old32) == read(executable));
    assert(DeleteFileW(old64.c_str()));
    assert(DeleteFileW(old32.c_str()));
    assert(RemoveDirectoryW(staging.c_str()));
    assert(RemoveDirectoryW(directory.c_str()));
    std::cout << "Updater failure, rollback, and replacement checks passed\n";
}
