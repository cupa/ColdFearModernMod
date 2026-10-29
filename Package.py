from hashlib import sha256
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

VERSION = "0.6.14"
ROOT = Path(__file__).resolve().parent
PLAYER_FILES = (
    "ColdFearThirdPerson.exe", "ColdFearModernControls.dll", "dinput8.dll",
    "Camera.ini", "ColdFearDisplay.ini", "Enable.cmd", "Disable.cmd",
    "Toggle.cmd", "Status.cmd", "README.md", "CONTROLS.md",
    "LICENSE.md", "INSTALL.md", "Uninstall.exe",
)
SOURCE_FILES = (
    "Build.cmd", "Test.cmd", "Package.py", "README.md", "CONTROLS.md",
    "BUILD.md", "LICENSE.md", "INSTALL.md", "BuildInstaller.cmd",
    "setup/Uninstall.h", "setup/Setup.cpp", "setup/TestSetup.cpp", "setup/Prepare.py",
    "setup/Setup.manifest", "setup/Payload.h", "setup/Setup.rc",
    "config/Camera.ini", "config/ColdFearDisplay.ini",
    "scripts/SetupMSVC.cmd", "scripts/Enable.cmd", "scripts/Disable.cmd",
    "scripts/Toggle.cmd", "scripts/Status.cmd",
    "src/ColdFearThirdPerson.cpp", "src/ModernControls.cpp",
    "src/ModernControls.h", "src/ModernControls.def", "src/Controller.h",
    "src/PlayStation.h", "src/TitlePrompt.h", "src/CursorConfinement.h",
    "src/DisplayPlugin.cpp", "src/DisplayPlugin.def", "src/DisplayModes.h",
    "src/TestControls.cpp", "src/TestController.cpp",
    "src/TestCursorConfinement.cpp", "src/TestDisplay.cpp",
)


def write_zip(path, root, names):
    for name in names:
        if not (root / name).is_file():
            raise FileNotFoundError(root / name)
    with ZipFile(path, "w", compression=ZIP_DEFLATED) as archive:
        for name in names:
            entry = ZipInfo(name, date_time=(2026, 10, 23, 0, 0, 0))
            entry.compress_type = ZIP_DEFLATED
            entry.create_system = 0
            archive.writestr(entry, (root / name).read_bytes())
    with ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise RuntimeError("Archive integrity check failed")


def main():
    destination = ROOT / "packages"
    destination.mkdir(exist_ok=True)
    player = destination / f"ColdFearModernThirdPerson-{VERSION}.zip"
    source = destination / f"ColdFearModernThirdPerson-{VERSION}-source.zip"
    write_zip(player, ROOT / "dist", PLAYER_FILES)
    write_zip(source, ROOT, SOURCE_FILES)
    archives = [player, source]
    installer = destination / "ColdFearModernThirdPerson-0.6.14-Setup.exe"
    if installer.is_file():
        installer_zip = destination / f"ColdFearModernThirdPerson-{VERSION}-installer.zip"
        with ZipFile(installer_zip, "w", compression=ZIP_DEFLATED) as archive:
            archive.write(installer, installer.name)
            archive.write(ROOT / "dist/Uninstall.exe", "Uninstall.exe")
            for name in ("INSTALL.md", "README.md", "CONTROLS.md", "LICENSE.md"):
                archive.write(ROOT / name, name)
        uninstaller = destination / "ColdFearModernThirdPerson-0.6.14-Uninstall.exe"
        uninstaller.write_bytes((ROOT / "dist/Uninstall.exe").read_bytes())
        archives.extend((installer, installer_zip, uninstaller))
    hashes = [f"{sha256(path.read_bytes()).hexdigest()}  {path.name}" for path in archives]
    (destination / "SHA256SUMS.txt").write_text("\n".join(hashes) + "\n", encoding="ascii")
    print("Created player ZIP, source ZIP and SHA256SUMS.txt in packages.")


if __name__ == "__main__":
    main()
