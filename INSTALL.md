# Install

Download the installer ZIP from [Releases](https://github.com/cupa/ColdFearModernMod/releases/latest).
Extract it, close the game and helper, then run the Setup EXE. Confirm the game
folder. The installer detects Steam or GOG and installs the appropriate files.
If both are installed, select the desired folder. Approve administrator access
if Windows requests it. Launch via Steam, or run `ColdFearModern.exe` for GOG.

For manual installation, extract the matching Steam or GOG player ZIP beside `coldfear_retail.exe`.
Back up existing files first. The GOG package keeps `dinput8.dll`, `dixi.ini`
and GOG resolution settings. Never install the Steam package over GOG.

Existing settings are preserved. The installer backs up replaced files in
`ColdFearModernBackup`. No game executable or assets are supplied.

## Uninstall

Close the game and helper. Run `Uninstall.exe` in the game folder or download the
standalone uninstall EXE. Confirm the folder. Recognized mod files are removed;
installer-preserved originals are restored. Extended display presets reset to
1024×768. Saves, backups, edited settings and unfamiliar files are kept.

Manual or older installations have no original-file record. Their recognized
files are removed without restoring earlier files. Logs and settings may remain.
Delete `Uninstall.exe` after it closes if desired.
