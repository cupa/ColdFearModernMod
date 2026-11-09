#define SETUP_TEST
#pragma warning(disable:4505)
#include "Setup.cpp"
#include <iostream>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void writeText(const fs::path& path,const char* text){std::ofstream out(path,std::ios::binary);out<<text;}
static std::string text(const fs::path& path){auto bytes=readFile(path);return {bytes.begin(),bytes.end()};}
static void testEdition(Edition selected) {
        selectEdition(selected);
        check(digest(reinterpret_cast<const BYTE*>("abc"),3)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA-256 mismatch");
        check(vdfValue("\"installdir\" \"Cold Fear\"","installdir")=="Cold Fear","manifest parse failed");
        fs::path root=fs::absolute(L"build")/(L"setup-test-"+std::to_wstring(GetTickCount64())+(selected==Edition::Gog?L"-gog":L"-steam"));
        fs::path fresh=root/L"fresh",upgrade=root/L"upgrade",failure=root/L"failure",conflict=root/L"conflict";
        for(const auto& folder:{fresh,upgrade,failure,conflict})fs::create_directories(folder);
        fs::path freshBackup=installFiles(fresh);
        for(const auto& item:payload)check(fileHash(fresh/item.name)==item.hash,"fresh install mismatch");
        if(selected==Edition::Gog)check(!fs::exists(fresh/L"dinput8.dll")&&!fs::exists(fresh/L"ColdFearDisplay.ini"),"Steam renderer installed over GOG");
        writeText(upgrade/L"Camera.ini","custom-camera");writeText(upgrade/L"ColdFearDisplay.ini","custom-display");
        writeText(upgrade/L"README.md","previous-readme");writeText(upgrade/L"res.bin","original-preset");
        fs::path backup=installFiles(upgrade);
        check(text(upgrade/L"Camera.ini")=="custom-camera"&&text(upgrade/L"ColdFearDisplay.ini")=="custom-display","settings were overwritten");
        check(text(backup/L"README.md")=="previous-readme"&&text(backup/L"res.bin")=="original-preset","backups were not preserved");
        fs::path secondBackup=installFiles(upgrade);check(secondBackup!=backup,"upgrade reused a backup directory");
        check(text(backup/L"README.md")=="previous-readme","upgrade changed an earlier backup");
        writeText(failure/L"ColdFearThirdPerson.exe","previous-helper");
        bool failed=false;try{installFiles(failure,1);}catch(const std::exception&){failed=true;}
        check(failed&&text(failure/L"ColdFearThirdPerson.exe")=="previous-helper"&&!fs::exists(failure/L"ColdFearModernControls.dll"),"partial install rollback failed");
        writeText(conflict/L"dinput8.dll","other-mod");failed=false;
        try{installFiles(conflict);}catch(const std::exception&){failed=true;}
        if(selected==Edition::Gog) {
            check(!failed&&text(conflict/L"dinput8.dll")=="other-mod","GOG input proxy was changed");
            auto graphics=root/L"graphics-conflict";fs::create_directories(graphics);
            writeText(graphics/L"d3d9.dll","other-graphics-mod");
            failed=false;try{installFiles(graphics);}catch(const std::exception&){failed=true;}
            check(failed&&text(graphics/L"d3d9.dll")=="other-graphics-mod"&&
                  !fs::exists(graphics/L"ColdFearThirdPerson.exe"),"conflicting GOG graphics DLL was changed");
        }
        else check(failed&&!fs::exists(conflict/L"ColdFearThirdPerson.exe")&&text(conflict/L"dinput8.dll")=="other-mod","conflicting proxy was changed");
        check(detectGame(conflict)==Edition::Unknown,"missing game was accepted");
        check(writable(fresh),"write-access probe failed");
        writeText(fresh/L"savegame.dat","save-data");
        DWORD extended=256;{std::ofstream out(fresh/L"res.bin",std::ios::binary);out.write(reinterpret_cast<const char*>(&extended),4);}
        auto removed=uninstallFiles(fresh);
        check(!fs::exists(fresh/L"dinput8.dll")&&!fs::exists(fresh/L"d3d9.dll")&&
              !fs::exists(fresh/L"ColdFearModernControls.dll")&&!fs::exists(fresh/L"ColdFearThirdPerson.exe"),"runtime files remain");
        check(text(fresh/L"savegame.dat")=="save-data"&&fs::exists(removed/L"ColdFearThirdPerson.exe"),"save or removal backup changed");
        auto preset=readFile(fresh/L"res.bin");DWORD value=0;memcpy(&value,preset.data(),4);
        if(selected==Edition::Gog)check(value==256,"GOG display preset was changed");
        else check(value==2,"extended preset not reset");
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
        if(selected==Edition::Gog)check(!failed&&text(conflict/L"dinput8.dll")=="other-mod","GOG removal changed input support");
        else check(failed&&text(conflict/L"dinput8.dll")=="other-mod","uninstall changed another proxy");
        if(selected==Edition::Gog)check(!fs::exists(fresh/L"ColdFearDisplay.ini")&&!fs::exists(fresh/L"dinput8.dll"),"Steam graphics files installed over GOG");
}
static void testLegacyLauncher(const fs::path& fixture) {
    selectEdition(Edition::Gog);
    auto folder=fs::absolute(L"build")/(L"legacy-launch-test-"+std::to_wstring(GetTickCount64()));
    fs::create_directories(folder);
    const auto launcher=folder/L"ColdFearModern.exe";
    fs::copy_file(fixture,launcher);
    check(legacyLauncher(launcher),"old launcher is not recognized");
    bool failed=false;try{installFiles(folder,1);}catch(const std::exception&){failed=true;}
    check(failed&&fileHash(launcher)==fileHash(fixture),"failed upgrade lost the old launcher");
    auto backup=installFiles(folder);
    check(!fs::exists(launcher)&&fileHash(backup/launcher.filename())==fileHash(fixture),"old launcher was not backed up and removed");
    writeText(launcher,"unrecognized launcher");
    installFiles(folder);uninstallFiles(folder);
    check(text(launcher)=="unrecognized launcher","unrecognized launcher was changed");
}
static void testWidescreen(const fs::path& fixture) {
    selectEdition(Edition::Steam);
    auto folder=fs::absolute(L"build")/(L"widescreen-test-"+std::to_wstring(GetTickCount64()));
    fs::create_directories(folder/L"scripts");
    for(auto name:{L"dinput8.dll",L"scripts/ColdFear.WidescreenFix.asi",L"scripts/ColdFear.WidescreenFix.ini"})
        fs::copy_file(fixture/name,folder/name);
    check(replacingWidescreen(folder),"official widescreen release not detected");
    const auto extra=folder/L"scripts"/L"OtherMod.asi";
    writeText(extra,"other mod");
    bool failed=false;try{installFiles(folder);}catch(const std::exception&){failed=true;}
    check(failed&&fileHash(folder/L"dinput8.dll")==widescreenLoader,"shared loader was replaced");
    fs::remove(extra);
    failed=false;try{installFiles(folder,4);}catch(const std::exception&){failed=true;}
    check(failed&&fileHash(folder/L"dinput8.dll")==widescreenLoader,"failed replacement did not restore the loader");
    auto backup=installFiles(folder);
    check(knownProxy(folder/L"dinput8.dll")&&fileHash(backup/L"dinput8.dll")==widescreenLoader,"loader was not replaced and backed up");
    installFiles(folder);uninstallFiles(folder);
    check(fileHash(folder/L"dinput8.dll")==widescreenLoader,"uninstall did not restore the widescreen loader");
    check(fileHash(folder/L"scripts/ColdFear.WidescreenFix.asi")==widescreenPlugin&&
          fileHash(folder/L"scripts/ColdFear.WidescreenFix.ini")==fileHash(fixture/L"scripts/ColdFear.WidescreenFix.ini"),
          "widescreen plugin or settings changed");
    std::cout<<"Widescreen replacement, shared-loader protection, rollback and restore passed.\n";
}
int main(int argc,char** argv) {
    try {
        if(argc==3&&!strcmp(argv[1],"--widescreen-fixture")){testWidescreen(fs::path(argv[2]));return 0;}
        if(argc==3&&!strcmp(argv[1],"--launcher-fixture")){testLegacyLauncher(fs::path(argv[2]));std::cout<<"Legacy launcher backup, removal and rollback passed.\n";return 0;}
        if(argc==2){auto found=detectGame(fs::path(argv[1]));check(found!=Edition::Unknown,"Unsupported game executable");std::cout<<(found==Edition::Gog?"GOG":"Steam")<<" detected. No files changed.\n";return 0;}
        verifyPayload();
        check(editionFromHash("b512f1d47fb85a41c784c71d987044f204cfec900fa9a61f28778dd05e351131")==Edition::Steam,"Steam detection failed");
        check(editionFromHash("1966fa18ecd6f08644cbedb1d4632373799fb03ed138a3d998e42ad713436cf1")==Edition::Gog,"GOG detection failed");
        check(editionFromHash("unknown")==Edition::Unknown,"Unknown version accepted");
        check(editionFromImageSize(0x4d6000)==Edition::Steam&&editionFromImageSize(0x4d4000)==Edition::Gog&&editionFromImageSize(0)==Edition::Unknown,"Runtime detection failed");
        bool rejected=false;try{selectEdition(Edition::Unknown);}catch(const std::exception&){rejected=true;}
        check(rejected,"Unsupported edition selected");
        testEdition(Edition::Steam);testEdition(Edition::Gog);
        std::cout<<"Steam/GOG detection, payload selection, backups, removal, rollback and settings checks passed.\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
