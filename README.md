# Cold Fear Modern Third Person

Third-person exploration, camera-relative movement, controller support and extra
resolutions for the Windows Steam version of Cold Fear.

See it in action here:
https://www.youtube.com/watch?v=iu89ArFWV0w

## Download and install

1. Download the **installer ZIP** from [Releases](https://github.com/cupa/ColdFearModernMod/releases/latest).
2. Extract it and close Cold Fear and its camera helper.
3. Run `ColdFearModernThirdPerson-0.6.14-Setup.exe` and confirm the game folder.
4. Launch through Steam. **F4** toggles the mod.

For manual installation, extract the player ZIP beside `coldfear_retail.exe`.
Do not overwrite another mod's `dinput8.dll`.

## Features

- Mouse/right-stick camera with movement relative to the camera.
- Adjustable camera distance and aim transition; 0.2 seconds by default.
- Keyboard, Xbox and native USB DualSense support, including menus.
- Original cutscene cameras and immediate camera placement after doors.
- 1080p, 1440p, 4K and 1280×800 resolution options.

## Controls

WASD moves, mouse looks, right mouse aims, left mouse shoots or shoves,
Shift runs, R reloads and Ctrl crouches. See [all controls](CONTROLS.md).
Edit `Camera.ini` and `ColdFearDisplay.ini` to configure the mod.

## Uninstall

Close the game and helper, then run `Uninstall.exe` in the game folder.
Saves and backups are kept. See [installation details](INSTALL.md).

## Compatibility

Only supports the Steam version of Cold Fear. The GOG version is untested.

Alt-Tab can cause white or corrupted graphics; return to the main menu and reload,
or restart. Tight spaces can cause camera clipping. Full-campaign coverage,
SteamOS/Proton and physical 4K/Steam Deck testing are incomplete.
Other executable versions and overlapping camera/input mods are unsupported.
The installer is unsigned; keep security protection enabled.

## Source

See [build instructions](BUILD.md). Licensed under [MIT](LICENSE.md).
Game files and assets are not included.

## Support

You can support me by donating at my [Kofi](https://ko-fi.com/leafbuckets01).
