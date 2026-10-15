#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <vector>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::vector<wchar_t> modulePath(32768);
    const DWORD pathLength = GetModuleFileNameW(
        nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (pathLength == 0 || pathLength >= modulePath.size()) {
        MessageBoxW(nullptr, L"Could not determine the game directory.",
                    L"Moons of Krewbull", MB_ICONERROR | MB_OK);
        return 1;
    }

    const std::wstring fullModulePath(modulePath.data(), pathLength);
    const std::wstring directory =
        fullModulePath.substr(0, fullModulePath.find_last_of(L"\\/"));
    const std::wstring runtimePath = directory + L"\\Moons of Krewbull.runtime";
    const std::wstring librariesPath = directory + L"\\libs";

    const DWORD currentPathLength = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    std::vector<wchar_t> currentPath(currentPathLength > 0 ? currentPathLength : 1);
    if (currentPathLength > 0) {
        GetEnvironmentVariableW(L"PATH", currentPath.data(), currentPathLength);
    } else {
        currentPath[0] = L'\0';
    }
    const std::wstring updatedPath = librariesPath + L";" + currentPath.data();
    if (!SetEnvironmentVariableW(L"PATH", updatedPath.c_str())) {
        MessageBoxW(nullptr, L"Could not configure the game libraries path.",
                    L"Moons of Krewbull", MB_ICONERROR | MB_OK);
        return 1;
    }

    std::wstring commandLine = L"\"" + runtimePath + L"\"";
    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(runtimePath.c_str(), commandLine.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, directory.c_str(),
                        &startupInfo, &processInfo)) {
        MessageBoxW(nullptr, L"Could not start the game executable.",
                    L"Moons of Krewbull", MB_ICONERROR | MB_OK);
        return 1;
    }

    CloseHandle(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
}
