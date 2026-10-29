#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <stdexcept>
#include "Payload.h"
namespace fs=std::filesystem;
static constexpr wchar_t title[]=L"Cold Fear Modern Third Person 0.6.14";
static constexpr char gameHash[]="b512f1d47fb85a41c784c71d987044f204cfec900fa9a61f28778dd05e351131";
struct Resource {const BYTE* bytes;DWORD size;};
static Resource resource(int id) {
    HMODULE module=GetModuleHandleW(nullptr);
    HRSRC found=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
    if(!found)throw std::runtime_error("Missing installer payload.");
    HGLOBAL loaded=LoadResource(module,found);
    auto bytes=static_cast<const BYTE*>(LockResource(loaded));
    DWORD size=SizeofResource(module,found);
    if(!bytes||!size)throw std::runtime_error("Invalid installer payload.");
    return {bytes,size};
}
static std::string digest(const BYTE* data,size_t size) {
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)
        throw std::runtime_error("SHA-256 is unavailable.");
    BYTE bytes[32]{};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=size<=MAXDWORD&&BCryptHashData(hash,const_cast<BYTE*>(data),static_cast<ULONG>(size),0)>=0;
    if(ok)ok=BCryptFinishHash(hash,bytes,sizeof(bytes),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(!ok)throw std::runtime_error("SHA-256 verification failed.");
    static constexpr char hex[]="0123456789abcdef";std::string result;
    for(BYTE value:bytes){result+=hex[value>>4];result+=hex[value&15];}
    return result;
}
static std::vector<BYTE> readFile(const fs::path& path) {
    std::ifstream input(path,std::ios::binary);
    if(!input)throw std::runtime_error("Cannot read a required file.");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
static std::string fileHash(const fs::path& path){auto bytes=readFile(path);return digest(bytes.data(),bytes.size());}
static void verifyPayload() {
    for(const auto& item:payload) {
        Resource data=resource(item.id);
        if(digest(data.bytes,data.size)!=item.hash)throw std::runtime_error("Installer payload checksum mismatch.");
    }
}
static bool supportedGame(const fs::path& folder) {
    fs::path game=folder/L"coldfear_retail.exe";
    return fs::is_regular_file(game)&&fileHash(game)==gameHash;
}
static bool knownProxy(const fs::path& path) {
    if(!fs::exists(path))return true;
    auto hash=fileHash(path);
    for(const char* allowed:knownProxies)if(hash==allowed)return true;
    return false;
}
static std::wstring wide(const std::string& text) {
    if(text.empty())return {};
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
    if(!count)return {};
    std::wstring result(count,L'\0');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),count);
    return result;
}
static std::string vdfValue(const std::string& text,const char* key) {
    std::regex pattern(std::string("\"")+key+"\"\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
    std::smatch match;return std::regex_search(text,match,pattern)?match[1].str():std::string{};
}
static fs::path libraryGame(const fs::path& library) {
    fs::path apps=library/L"steamapps",manifest=apps/L"appmanifest_15270.acf";
    if(!fs::is_regular_file(manifest))return {};
    auto bytes=readFile(manifest);std::string text(bytes.begin(),bytes.end());
    auto directory=wide(vdfValue(text,"installdir"));
    fs::path result=apps/L"common"/directory;
    return !directory.empty()&&fs::is_regular_file(result/L"coldfear_retail.exe")?result:fs::path{};
}
static fs::path discover() {
    wchar_t steam[32768]{};DWORD bytes=sizeof(steam);
    if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,nullptr,steam,&bytes)!=ERROR_SUCCESS)return {};
    fs::path root=steam,found=libraryGame(root);if(!found.empty())return found;
    fs::path list=root/L"steamapps"/L"libraryfolders.vdf";
    if(!fs::is_regular_file(list))return {};
    auto data=readFile(list);std::string text(data.begin(),data.end());
    std::regex pattern("\"path\"\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
    for(std::sregex_iterator i(text.begin(),text.end(),pattern),end;i!=end;++i) {
        std::string path=(*i)[1].str();
        for(size_t p=0;(p=path.find("\\\\",p))!=std::string::npos;++p)path.replace(p,2,"\\");
        found=libraryGame(wide(path));if(!found.empty())return found;
    }
    return {};
}
static fs::path browse() {
    wchar_t display[MAX_PATH]{},path[MAX_PATH]{};BROWSEINFOW info{};
    info.pszDisplayName=display;info.lpszTitle=L"Select the Cold Fear folder containing coldfear_retail.exe";
    info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE result=SHBrowseForFolderW(&info);if(!result)return {};
    bool ok=SHGetPathFromIDListW(result,path)!=FALSE;CoTaskMemFree(result);
    return ok?fs::path(path):fs::path{};
}
static bool gameRunning() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot check running game processes.");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);bool running=false;
    if(Process32FirstW(snapshot,&entry))do {
        if(!_wcsicmp(entry.szExeFile,L"coldfear_retail.exe")||!_wcsicmp(entry.szExeFile,L"ColdFearThirdPerson.exe")){running=true;break;}
    }while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);return running;
}
static bool writable(const fs::path& folder) {
    fs::path probe=folder/(L".cfmodern-write-"+std::to_wstring(GetCurrentProcessId()));
    HANDLE file=CreateFileW(probe.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    CloseHandle(file);return true;
}
struct Installed {fs::path file,backup;bool existed;};
static void plainPath(const fs::path& path) {
    DWORD attributes=GetFileAttributesW(path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))
        throw std::runtime_error("A mod or backup path is a link. Restore it to a regular file or folder first.");
}
static fs::path installFiles(const fs::path& folder,int failAfter=-1) {
    if(!knownProxy(folder/L"dinput8.dll"))throw std::runtime_error("Another dinput8.dll is installed. Use a clean game folder; this installer cannot chain another mod.");
    plainPath(folder);plainPath(folder/L"ColdFearModernBackup");
    fs::path baseline=folder/L"ColdFearModernBackup"/L"Original";
    plainPath(baseline);
    if(!fs::exists(baseline/L"complete")) {
        fs::create_directories(baseline);
        for(const auto& item:payload) {
            fs::path target=folder/item.name;plainPath(target);plainPath(baseline/item.name);
            if(fs::is_regular_file(target)&&fileHash(target)!=item.hash&&
               _wcsicmp(item.name,L"dinput8.dll")&&_wcsicmp(item.name,L"ColdFearThirdPerson.exe")&&
               _wcsicmp(item.name,L"ColdFearModernControls.dll")&&_wcsicmp(item.name,L"Uninstall.exe"))
                fs::copy_file(target,baseline/item.name,fs::copy_options::overwrite_existing);
        }
        std::ofstream marker(baseline/L"complete");marker<<"0.6.14\n";marker.flush();
        if(!marker)throw std::runtime_error("Cannot record original files.");
    }
    for(const auto& item:payload)plainPath(folder/item.name);
    SYSTEMTIME now{};GetLocalTime(&now);wchar_t label[96]{};
    swprintf_s(label,L"0.6.14-%04u%02u%02u-%02u%02u%02u-%lu",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,GetCurrentProcessId());
    fs::path parent=folder/L"ColdFearModernBackup";fs::create_directories(parent);
    fs::path backup;
    for(unsigned sequence=0;;++sequence) {
        backup=parent/(std::wstring(label)+L"-"+std::to_wstring(sequence));
        if(fs::create_directory(backup))break;
    }
    std::vector<Installed> completed;
    try {
        if(fs::is_regular_file(folder/L"res.bin"))fs::copy_file(folder/L"res.bin",backup/L"res.bin",fs::copy_options::overwrite_existing);
        for(const auto& item:payload) {
            fs::path target=folder/item.name;
            bool exists=fs::exists(target);
            if(exists&&(!_wcsicmp(item.name,L"Camera.ini")||!_wcsicmp(item.name,L"ColdFearDisplay.ini")))continue;
            fs::path original=backup/item.name;
            if(exists)fs::copy_file(target,original,fs::copy_options::overwrite_existing);
            Resource data=resource(item.id);
            if(failAfter==static_cast<int>(completed.size()))throw std::runtime_error("Test write failure.");
            completed.push_back({target,original,exists});
            std::ofstream output(target,std::ios::binary|std::ios::trunc);
            if(!output||!output.write(reinterpret_cast<const char*>(data.bytes),data.size)||!output.flush())
                throw std::runtime_error("Could not install a file. Close the game and check folder permissions.");
        }
    }catch(...) {
        bool restored=true;
        for(auto i=completed.rbegin();i!=completed.rend();++i) {
            std::error_code error;
            if(i->existed)fs::copy_file(i->backup,i->file,fs::copy_options::overwrite_existing,error);
            else fs::remove(i->file,error);
            if(error)restored=false;
        }
        if(!restored)throw std::runtime_error("Install failed and some files could not be restored. Restore the files in ColdFearModernBackup before playing.");
        throw;
    }
    return backup;
}
#include "Uninstall.h"
#ifndef SETUP_TEST
static int run() {
    int count=0;wchar_t** args=CommandLineToArgvW(GetCommandLineW(),&count);fs::path folder;bool verify=false;
    if(!args)throw std::runtime_error("Cannot read installer arguments.");
    for(int i=1;i<count;++i) {
        if(!wcscmp(args[i],L"--verify"))verify=true;
        else if(!wcscmp(args[i],L"--folder")&&i+1<count)folder=args[++i];
    }
    LocalFree(args);
#ifdef UNINSTALLER
    if(verify)return 0;
    return uninstallInteractive(folder);
#else
    verifyPayload();if(verify)return 0;
    if(folder.empty())folder=discover();
    if(!folder.empty()) {
        std::wstring question=L"Install Cold Fear Modern Third Person to:\n\n"+folder.wstring()+L"\n\nYes: install here\nNo: choose another folder\nCancel: exit\n\nExisting settings are preserved. Replaced files are backed up.";
        int choice=MessageBoxW(nullptr,question.c_str(),title,MB_YESNOCANCEL|MB_ICONQUESTION);
        if(choice==IDCANCEL)return 0;if(choice==IDNO)folder.clear();
    }
    if(folder.empty()) {
        folder=browse();if(folder.empty())return 0;
        std::wstring question=L"Install to:\n\n"+folder.wstring()+L"\n\nExisting settings are preserved. Replaced files are backed up.";
        if(MessageBoxW(nullptr,question.c_str(),title,MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return 0;
    }
    if(!supportedGame(folder))throw std::runtime_error("Select an unmodified supported Steam installation of Cold Fear. The executable checksum did not match.");
    if(gameRunning())throw std::runtime_error("Close Cold Fear and its camera helper, then run this installer again. No game files were changed.");
    if(!knownProxy(folder/L"dinput8.dll"))throw std::runtime_error("Another dinput8.dll is installed. This installer cannot replace a different input proxy.");
    if(!writable(folder)) {
        if(IsUserAnAdmin())throw std::runtime_error("The game folder is not writable. Check its permissions and available disk space.");
        wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);
        std::wstring parameters=L"--folder \""+folder.wstring()+L"\"";
        auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"runas",self,parameters.c_str(),nullptr,SW_SHOWNORMAL));
        if(result<=32)throw std::runtime_error("Installation needs write access to the game folder. The elevation request was canceled or could not start.");
        return 0;
    }
    fs::path backup=installFiles(folder);
    std::wstring message=L"Installation complete.\n\nLaunch Cold Fear through Steam. F4 toggles the mod.\n\nBackups:\n"+backup.wstring()+L"\n\nRead README.md for controls, known issues and removal instructions.";
    MessageBoxW(nullptr,message.c_str(),title,MB_OK|MB_ICONINFORMATION);return 0;
#endif
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);int result=1;
    try {result=run();}
    catch(const std::exception& error){auto message=wide(error.what());MessageBoxW(nullptr,message.c_str(),title,MB_OK|MB_ICONERROR);}
    if(SUCCEEDED(com))CoUninitialize();return result;
}
#endif
