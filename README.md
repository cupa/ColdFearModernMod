# Cold Fear Modern Third Person

Third-person exploration, camera-relative movement, controller support and extra
resolutions for Cold Fear on Windows (Steam and GOG).

See it in action here:
https://www.youtube.com/watch?v=iu89ArFWV0w

## Download and install

1. Download the **installer ZIP** from [Releases](https://github.com/cupa/ColdFearModernMod/releases/latest).
2. Extract it and close Cold Fear and its camera helper.
3. Run the Setup EXE and confirm the game folder. Steam or GOG is detected automatically.
4. Launch Cold Fear as usual from Steam, GOG Galaxy, or the GOG shortcut. **F4** toggles the mod.

To update, close the game and run the new installer over the same folder.
Setup keeps your saves and settings. See [upgrade details](INSTALL.md#update).

For manual installation, extract the matching Steam or GOG player ZIP beside `coldfear_retail.exe`.
The GOG package keeps GOG's input and display fixes.
Steam Setup can replace the official Cold Fear Widescreen Fix and restore its loader on uninstall.

## Features

### Steam and GOG

- Third-person exploration with mouse or right-stick camera and camera-relative movement.
- Adjustable camera distance and aim transition.
- Original cutscene cameras and immediate camera placement after doors.
- Xbox and native USB DualSense support with the mod’s button mappings.
- Title prompt shows “Press any button to START” after controller input and returns to the original wording after mouse or keyboard input.

### Steam

- Controller menu navigation.
- Automatic desktop resolution, with 1080p, 1440p, 4K, and 1280×800 presets and custom overrides.
- Widescreen HUD scaling, aspect-aware field of view, fit or fill video, and optional cutscene border removal.
- Alt-Tab graphics fix for white corruption and broken textures.

### GOG

- Uses GOG’s controller wrapper alongside native DualSense input.
- Keeps GOG’s built-in widescreen, resolution, and Alt-Tab fixes.

## Controls

WASD moves, mouse looks, right mouse aims, left mouse shoots or shoves,
Shift runs, R reloads and Ctrl crouches. See [all controls](CONTROLS.md).
Edit `Camera.ini` to configure the camera and controls. Steam display settings use `ColdFearDisplay.ini`. See [widescreen settings](INSTALL.md#steam-widescreen-settings).

## Uninstall

Close the game and helper, then run `Uninstall.exe` in the game folder.
Saves and backups are kept. See [installation details](INSTALL.md).

## Compatibility

Targets the original Steam build and the GOG Preservation build.

Tight spaces can cause
camera clipping. Full-campaign coverage,
SteamOS/Proton and physical 4K/Steam Deck testing are incomplete.
Other executable versions and overlapping camera/input mods are unsupported.
The installer is unsigned; keep security protection enabled.

## Source

See [build instructions](BUILD.md). Licensed under [MIT](LICENSE.md).
Game files and assets are not included.

## Support

You can support me by donating at my [Kofi](https://ko-fi.com/leafbuckets01).
