# Install

Download the installer ZIP from [Releases](https://github.com/cupa/ColdFearModernMod/releases/latest).
Extract it, close the game and helper, then run the Setup EXE. Confirm the game
folder. The installer detects Steam or GOG and installs the appropriate files.
If both are installed, select the desired folder. Approve administrator access
if Windows requests it. Start Cold Fear from Steam, GOG Galaxy, or the GOG shortcut.

For manual installation, extract the matching Steam or GOG player ZIP beside `coldfear_retail.exe`.
Back up existing files first. Use the package for your edition. GOG uses
`d3d9.dll` to start the mod and keeps GOG’s input and display settings.
If you already have a `d3d9.dll` from another mod, back it up and move it out
of the game folder before installing.

## Update

Close Cold Fear and run the latest Setup EXE. Select the same game folder,
even if you installed the previous version manually. Setup keeps your saves
and settings, backs up replaced files in `ColdFearModernBackup`, and moves the
old `ColdFearModern.exe` there if it recognizes it. Use the normal game shortcut
after updating.

## Steam widescreen settings

Steam starts at the current desktop resolution until you choose a mode in
**Settings > Screen configuration**. Accept and restart to apply your choice.
Steam Setup replaces the official Cold Fear Widescreen Fix loader if no other
ASI plugins use it. Uninstall restores the backed-up loader. The widescreen
fix's plugin and settings are kept. Other loaders require manual removal.

In `ColdFearDisplay.ini`:

- Under `[Display]`, `UseDesktopResolution=-1` is automatic. Use `1` to
  always follow the desktop or `0` to honor even an old stock preset.
  Set `ResolutionWidth` and `ResolutionHeight` to force a supported mode.
- Under `[Widescreen]`, `CorrectHUD` and `CorrectFOV` default to 1.
  Set `HUDAtEdges=0` for a centered HUD, or adjust `HUDEdgePercent` (0–100).
- Set `VideoMode=0` for original framing, `1` to fit, or `2` to fill
  the screen with a centered crop.
- Set `RemoveCutsceneBorders=1` to hide in-game cutscene bars.

Restart after editing. Existing INI files are kept during upgrades.
In `Camera.ini`, use `KeepCursorInGame` to toggle cursor confinement.
Older cursor settings still work. GOG uses its own display and input support.

## Uninstall

Close the game and helper. Run `Uninstall.exe` in the game folder or download the
standalone uninstall EXE. Confirm the folder. Recognized mod files are removed;
installer-preserved originals are restored. Extended display presets reset to
1024×768. Saves, backups, edited settings and unfamiliar files are kept.

Manual or older installations have no original-file record. Their recognized
files are removed without restoring earlier files. Logs and settings may remain.
Delete `Uninstall.exe` after it closes if desired.
