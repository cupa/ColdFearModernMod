#pragma once
struct Removal {fs::path file,original;bool restore;};
static bool runtimeFile(const wchar_t* name) {
    return !_wcsicmp(name,L"dinput8.dll")||!_wcsicmp(name,L"ColdFearModernControls.dll")||!_wcsicmp(name,L"ColdFearThirdPerson.exe");
}
static fs::path uninstallFiles(const fs::path& folder,int failAfter=-1) {
    plainPath(folder);plainPath(folder/L"ColdFearModernBackup");
    fs::path baseline=folder/L"ColdFearModernBackup"/L"Original";plainPath(baseline);
    std::vector<Removal> changes;
    for(const auto& item:payload) {
        if(!_wcsicmp(item.name,L"Uninstall.exe"))continue;
        fs::path target=folder/item.name,original=baseline/item.name;
        plainPath(target);plainPath(original);
        if(!fs::exists(target))continue;
        bool matches=fileHash(target)==item.hash;
        if(runtimeFile(item.name)&&!matches) {
            if(!_wcsicmp(item.name,L"dinput8.dll")&&knownProxy(target))matches=true;
            else throw std::runtime_error("A runtime file is unfamiliar or modified. No files were removed. Restore this mod's original files before uninstalling.");
        }
        bool restore=fs::is_regular_file(baseline/L"complete")&&fs::is_regular_file(original);
        if(matches)changes.push_back({target,original,restore});
    }
    fs::path resolution=folder/L"res.bin";plainPath(resolution);bool reset=false;
    if(fs::is_regular_file(resolution)) {
        auto data=readFile(resolution);
        if(data.size()==sizeof(DWORD)) {DWORD value=0;memcpy(&value,data.data(),sizeof(value));reset=value>4;}
    }
    fs::create_directories(folder/L"ColdFearModernBackup");
    fs::path backup;
    for(unsigned sequence=0;;++sequence) {
        backup=folder/L"ColdFearModernBackup"/(L"Uninstall-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(sequence));
        if(fs::create_directory(backup))break;
    }
    for(const auto& item:changes)fs::copy_file(item.file,backup/item.file.filename());
    if(reset)fs::copy_file(resolution,backup/L"res.bin");
    std::vector<fs::path> completed;
    try {
        for(const auto& item:changes) {
            if(failAfter==static_cast<int>(completed.size()))throw std::runtime_error("Test removal failure.");
            completed.push_back(item.file);
            if(item.restore)fs::copy_file(item.original,item.file,fs::copy_options::overwrite_existing);
            else fs::remove(item.file);
        }
        if(reset) {
            completed.push_back(resolution);
            DWORD stock=2;std::ofstream out(resolution,std::ios::binary|std::ios::trunc);
            out.write(reinterpret_cast<const char*>(&stock),sizeof(stock));out.flush();
            if(!out)throw std::runtime_error("Cannot reset the saved display preset.");
        }
    }catch(...) {
        bool restored=true;
        for(const auto& file:completed) {
            std::error_code error;fs::copy_file(backup/file.filename(),file,fs::copy_options::overwrite_existing,error);
            if(error)restored=false;
        }
        if(!restored)throw std::runtime_error("Removal failed. Restore files from the newest Uninstall folder in ColdFearModernBackup before playing.");
        throw;
    }
    return backup;
}
[[maybe_unused]] static int uninstallInteractive(fs::path folder) {
    wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);
    if(folder.empty()&&fs::is_regular_file(fs::path(self).parent_path()/L"coldfear_retail.exe"))folder=fs::path(self).parent_path();
    if(folder.empty())folder=discover();
    if(folder.empty())folder=browse();
    if(folder.empty())return 0;
    if(!supportedGame(folder))throw std::runtime_error("Select the supported Steam Cold Fear game folder.");
    std::wstring question=L"Uninstall Cold Fear Modern Third Person from:\n\n"+folder.wstring()+L"\n\nSaves and backups are kept. Modified settings and unfamiliar files are kept. An extended display preset is reset to 1024 x 768.";
    if(MessageBoxW(nullptr,question.c_str(),title,MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return 0;
    if(gameRunning())throw std::runtime_error("Close Cold Fear and its camera helper, then run the uninstaller again.");
    if(!writable(folder)) {
        if(IsUserAnAdmin())throw std::runtime_error("The game folder is not writable.");
        std::wstring parameters=L"--folder \""+folder.wstring()+L"\"";
        auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"runas",self,parameters.c_str(),nullptr,SW_SHOWNORMAL));
        if(result<=32)throw std::runtime_error("Uninstall needs write access. The elevation request was canceled or could not start.");
        return 0;
    }
    auto backup=uninstallFiles(folder);
    std::wstring message=L"Mod removed. Saves and backups have been kept.\n\nRemoval backup:\n"+backup.wstring()+L"\n\nYou can delete Uninstall.exe after closing this window. Modified settings, logs and unfamiliar files may remain.";
    MessageBoxW(nullptr,message.c_str(),title,MB_OK|MB_ICONINFORMATION);return 0;
}
