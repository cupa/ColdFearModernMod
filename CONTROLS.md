# Controls

## Keyboard and mouse

| Action | Input |
| --- | --- |
| Move relative to camera | WASD |
| Turn camera | Mouse |
| Aim | Right mouse |
| Shoot / shove without aiming | Left mouse |
| Run | Hold Shift |
| Reload | R |
| Crouch | Ctrl or C |
| Interact | E |
| Toggle modern camera/controls | F4 |

Other actions retain the game's bindings. Native stamina, ammunition, melee
eligibility and movement rules still apply.

## Controller

| Action | Xbox | PlayStation |
| --- | --- | --- |
| Move / look | Left / right stick | Left / right stick |
| Aim | LT | L2 |
| Shoot / shove without aiming | RT | R2 |
| Interact / select menu item | A | Cross |
| Reload | X | Square |
| Crouch / back in menus | Right-stick click or B | R3 or Circle |
| Run | Hold LB or L3 | Hold L1 or L3 |
| Cling / grab ledge | RB | R1 |
| Next weapon | D-pad right or Y | D-pad right or Triangle |
| Previous weapon | D-pad left | D-pad left |
| Pause only | Start/Menu | Options |
| Open menu | View/Back | Touchpad click or Create/Share |
| Navigate menus | D-pad or left stick | D-pad or left stick |

At the title screen, use A/Cross or Start/Options to enter. The controller prompt
changes on stick/button activity; mouse or keyboard input restores the stock text.

Xbox/XInput controllers work directly. Supported Sony IDs cover DualSense,
DualSense Edge and DualShock 4. USB DualSense is hardware-tested; other Sony
models and Bluetooth layouts still need hardware testing. Native Sony input
does not require Steam Input. Release controls once after connecting, then move
a stick. PlayerIndex=-1 lets either connected controller take over on fresh input.

For other controllers, try Steam Input with a standard Gamepad layout. A
keyboard/mouse emulation layout can interfere with input selection. Controller
support does not include rumble, gyro, adaptive triggers or new button glyphs.

Sony report layouts were referenced against the
hid-playstation driver (https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c).
No driver source is bundled. See Valve's documentation for
supported Steam Input devices (https://partner.steamgames.com/doc/features/steam_controller/device).
