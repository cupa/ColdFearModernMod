#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <fstream>
#include <shellapi.h>
#include "GameEdition.h"
namespace fs=std::filesystem;
static DWORD existingGame(const fs::path& game) {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot check running games.");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);DWORD found=0;
    if(Process32FirstW(snapshot,&entry))do {
        if(_wcsicmp(entry.szExeFile,L"coldfear_retail.exe"))continue;
        HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);
        wchar_t path[32768]{};DWORD size=32768;
        if(process&&QueryFullProcessImageNameW(process,0,path,&size)&&!_wcsicmp(path,game.c_str()))found=entry.th32ProcessID;
        if(process)CloseHandle(process);
    }while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);return found;
}
static DWORD launch(const fs::path& program,const fs::path& folder,const std::wstring& arguments,DWORD flags) {
    std::wstring command=L"\""+program.wstring()+L"\" "+arguments;
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    if(!CreateProcessW(program.c_str(),command.data(),nullptr,nullptr,FALSE,flags,nullptr,folder.c_str(),&startup,&child))
        throw std::runtime_error("Cannot start the game or camera helper. Check the extracted mod files.");
    DWORD pid=child.dwProcessId;CloseHandle(child.hThread);CloseHandle(child.hProcess);return pid;
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try {
        wchar_t path[32768]{};if(!GetModuleFileNameW(nullptr,path,32768))return 1;
        fs::path folder=fs::path(path).parent_path(),game=folder/L"ColdFear_retail.exe";
        for(const auto& name:{L"ColdFear_retail.exe",L"ColdFearThirdPerson.exe",L"ColdFearModernControls.dll"})
            if(!fs::is_regular_file(folder/name))throw std::runtime_error("Install this mod into the Cold Fear game folder first.");
        std::ifstream input(game,std::ios::binary);IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 nt{};
        if(!input.read(reinterpret_cast<char*>(&dos),sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>0x100000)
            throw std::runtime_error("Unsupported Cold Fear executable.");
        input.seekg(dos.e_lfanew);input.read(reinterpret_cast<char*>(&nt),sizeof(nt));
        Edition edition=input&&nt.Signature==IMAGE_NT_SIGNATURE?editionFromImageSize(nt.OptionalHeader.SizeOfImage):Edition::Unknown;
        if(edition==Edition::Unknown)throw std::runtime_error("Unsupported Cold Fear executable.");
        DWORD pid=existingGame(game);
        if(!pid&&edition==Edition::Steam) {
            if(reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"open",L"steam://rungameid/15270",nullptr,folder.c_str(),SW_SHOWNORMAL))<=32)
                throw std::runtime_error("Cannot launch Cold Fear through Steam.");
            return 0;
        }
        if(!pid)pid=launch(game,folder,L"",0);
        std::wstring mutex=L"Local\\ColdFearThirdPerson.Instance."+std::to_wstring(pid);
        HANDLE running=OpenMutexW(SYNCHRONIZE,FALSE,mutex.c_str());if(running){CloseHandle(running);return 0;}
        launch(folder/L"ColdFearThirdPerson.exe",folder,L"--enable --wait-ready --pid "+std::to_wstring(pid),CREATE_NO_WINDOW);
        return 0;
    }catch(const std::exception& error) {
        MessageBoxA(nullptr,error.what(),"Cold Fear Modern Third Person",MB_OK|MB_ICONERROR);return 1;
    }
}
