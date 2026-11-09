#pragma once
#include <windows.h>
#include <cwchar>

inline bool launchCameraHelper(const wchar_t* folder,DWORD& error) {
    error=ERROR_SUCCESS;
    wchar_t mutexName[128];
    swprintf_s(mutexName,L"Local\\ColdFearThirdPerson.Instance.%lu",GetCurrentProcessId());
    HANDLE existing=OpenMutexW(SYNCHRONIZE,FALSE,mutexName);
    if(existing){CloseHandle(existing);return false;}

    wchar_t helper[MAX_PATH];
    if(swprintf_s(helper,L"%s\\ColdFearThirdPerson.exe",folder)<0)return false;
    DWORD attributes=GetFileAttributesW(helper);
    if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_DIRECTORY))return false;

    wchar_t command[MAX_PATH+96];
    swprintf_s(command,L"\"%s\" --enable --wait-ready --pid %lu",helper,GetCurrentProcessId());
    STARTUPINFOW startup{};
    startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    if(!CreateProcessW(helper,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,folder,&startup,&process)){
        error=GetLastError();
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
