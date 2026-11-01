# Cold Fear Modern Third Person

Third-person exploration, camera-relative movement, controller support and extra
resolutions for Cold Fear on Windows (Steam and GOG).

See it in action here:
https://www.youtube.com/watch?v=iu89ArFWV0w

## Download and install

1. Download the **installer ZIP** from [Releases](https://github.com/cupa/ColdFearModernMod/releases/latest).
2. Extract it and close Cold Fear and its camera helper.
3. Run the Setup EXE and confirm the game folder. Steam or GOG is detected automatically.
4. Steam: launch through Steam. GOG: run `ColdFearModern.exe`. **F4** toggles the mod.

For manual installation, extract the matching Steam or GOG player ZIP beside `coldfear_retail.exe`.
The GOG package keeps GOG's input and display fixes.
Do not install the Steam package over GOG or another input proxy.

## Features

- Mouse/right-stick camera with movement relative to the camera.
- Adjustable camera distance and aim transition; 0.2 seconds by default.
- Keyboard, Xbox and native USB DualSense support, including menus.
- Original cutscene cameras and immediate camera placement after doors.
- 1080p, 1440p, 4K and 1280×800 resolution options.
- Steam Alt-Tab graphics fix for white corruption and broken textures.

## Controls

WASD moves, mouse looks, right mouse aims, left mouse shoots or shoves,
Shift runs, R reloads and Ctrl crouches. See [all controls](CONTROLS.md).
Edit `Camera.ini` to configure the camera and controls. Steam display settings use `ColdFearDisplay.ini`.

## Uninstall

Close the game and helper, then run `Uninstall.exe` in the game folder.
Saves and backups are kept. See [installation details](INSTALL.md).

## Compatibility

Targets the original Steam build and the GOG Preservation build.
GOG uses its existing input wrapper; controller mappings follow this mod's layout.

Steam includes the verified Alt-Tab graphics correction. Tight spaces can cause
camera clipping. Full-campaign coverage,
SteamOS/Proton and physical 4K/Steam Deck testing are incomplete.
Other executable versions and overlapping camera/input mods are unsupported.
The installer is unsigned; keep security protection enabled.

## Source

See [build instructions](BUILD.md). Licensed under [MIT](LICENSE.md).
Game files and assets are not included.

## Support

You can support me by donating at my [Kofi](https://ko-fi.com/leafbuckets01).
