#define SETUP_TEST
#pragma warning(disable:4505)
#include "Setup.cpp"
#include <iostream>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void writeText(const fs::path& path,const char* text){std::ofstream out(path,std::ios::binary);out<<text;}
static std::string text(const fs::path& path){auto bytes=readFile(path);return {bytes.begin(),bytes.end()};}
int main() {
    try {
        verifyPayload();
        check(digest(reinterpret_cast<const BYTE*>("abc"),3)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA-256 mismatch");
        check(vdfValue("\"installdir\" \"Cold Fear\"","installdir")=="Cold Fear","manifest parse failed");
        fs::path root=fs::absolute(L"build")/(L"setup-test-"+std::to_wstring(GetTickCount64()));
        fs::path fresh=root/L"fresh",upgrade=root/L"upgrade",failure=root/L"failure",conflict=root/L"conflict";
        for(const auto& folder:{fresh,upgrade,failure,conflict})fs::create_directories(folder);
        fs::path freshBackup=installFiles(fresh);
        for(const auto& item:payload)check(fileHash(fresh/item.name)==item.hash,"fresh install mismatch");
        writeText(upgrade/L"Camera.ini","custom-camera");writeText(upgrade/L"ColdFearDisplay.ini","custom-display");
        writeText(upgrade/L"README.md","previous-readme");writeText(upgrade/L"res.bin","original-preset");
        fs::path backup=installFiles(upgrade);
        check(text(upgrade/L"Camera.ini")=="custom-camera"&&text(upgrade/L"ColdFearDisplay.ini")=="custom-display","settings were overwritten");
        check(text(backup/L"README.md")=="previous-readme"&&text(backup/L"res.bin")=="original-preset","backups were not preserved");
        fs::path secondBackup=installFiles(upgrade);check(secondBackup!=backup,"upgrade reused a backup directory");
        check(text(backup/L"README.md")=="previous-readme","upgrade changed an earlier backup");
        writeText(failure/L"ColdFearThirdPerson.exe","previous-helper");
        bool failed=false;try{installFiles(failure,2);}catch(const std::exception&){failed=true;}
        check(failed&&text(failure/L"ColdFearThirdPerson.exe")=="previous-helper"&&!fs::exists(failure/L"ColdFearModernControls.dll"),"partial install rollback failed");
        writeText(conflict/L"dinput8.dll","other-mod");failed=false;
        try{installFiles(conflict);}catch(const std::exception&){failed=true;}
        check(failed&&!fs::exists(conflict/L"ColdFearThirdPerson.exe")&&text(conflict/L"dinput8.dll")=="other-mod","conflicting proxy was changed");
        check(!supportedGame(conflict),"missing game was accepted");
        check(writable(fresh),"write-access probe failed");
        writeText(fresh/L"savegame.dat","save-data");
        DWORD extended=256;{std::ofstream out(fresh/L"res.bin",std::ios::binary);out.write(reinterpret_cast<const char*>(&extended),4);}
        auto removed=uninstallFiles(fresh);
        check(!fs::exists(fresh/L"dinput8.dll")&&!fs::exists(fresh/L"ColdFearModernControls.dll")&&!fs::exists(fresh/L"ColdFearThirdPerson.exe"),"runtime files remain");
        check(text(fresh/L"savegame.dat")=="save-data"&&fs::exists(removed/L"dinput8.dll"),"save or removal backup changed");
        auto preset=readFile(fresh/L"res.bin");DWORD value=0;memcpy(&value,preset.data(),4);check(value==2,"extended preset not reset");
        writeText(upgrade/L"ColdFearDisplay.ini","changed-display");
        uninstallFiles(upgrade);
        check(text(upgrade/L"README.md")=="previous-readme"&&text(upgrade/L"Camera.ini")=="custom-camera","original files not restored across upgrades");
        check(text(upgrade/L"ColdFearDisplay.ini")=="changed-display","edited preexisting settings changed");
        installFiles(failure);failed=false;
        try{uninstallFiles(failure,2);}catch(const std::exception&){failed=true;}
        for(const auto& item:payload)check(fileHash(failure/item.name)==item.hash,"uninstall rollback mismatch");
        check(failed,"uninstall failure not injected");
        writeText(failure/L"Camera.ini","edited-camera");uninstallFiles(failure);
        check(text(failure/L"Camera.ini")=="edited-camera","modified settings removed");
        failed=false;try{uninstallFiles(conflict);}catch(const std::exception&){failed=true;}
        check(failed&&text(conflict/L"dinput8.dll")=="other-mod","uninstall changed another proxy");
        std::cout<<"Installer and uninstaller resources, backups, upgrades, rollback, saves, settings and conflict checks passed.\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
