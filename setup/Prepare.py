from hashlib import sha256
from pathlib import Path
import importlib.util
import sys
sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("release_package", ROOT / "Package.py")
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)
previous_proxies = (
    "f4763d63b1ca672f792509c3e9d84a9e41c94673aa988022f9d05c295ea003d3",
    "8b8800b12a1a808e5392bd8537a766428c4c15cf55d9e19a88820c0965e6fdc9",
    "c4678759f86e3f93ed07367a0782770fc77261b8629575a22d648722a0c5223c",
    "2ae3afc958d16ef6ae9010af9ebf801587e81744599851110a94ebd4655d781b",
    "d0010f078616b0d8c4fe82f389818f7f2436929b8c8a15a164a255f7807f1b39",
    "ad897be3c24be9256b2bdfc6a01ae2fe0a00e4a5c2ff3237fa5b56e8c930e505",
    "f067bff4edbf882b62fbca0be9ad5b956b136f9585e10e7ef2c06fd1271890c7",
)
header = ["#pragma once", "struct PayloadFile {int id;const wchar_t* name;const char* hash;};"]
resources = ['#include <windows.h>', '1 RT_MANIFEST "setup\\\\Setup.manifest"']
for label, folder, names, start in (
    ("steam", ROOT / "dist", package.PLAYER_FILES, 101),
    ("gog", ROOT / "dist/gog", package.GOG_FILES, 201),
):
    header.append(f"static constexpr PayloadFile {label}Payload[]={{")
    for number, name in enumerate(names, start):
        if name == "Uninstall.exe" and "--uninstaller" in sys.argv:
            continue
        path = folder / name
        if not path.is_file():
            raise FileNotFoundError(path)
        digest = sha256(path.read_bytes()).hexdigest()
        header.append(f'    {{{number},L"{name}","{digest}"}},')
        relative = path.relative_to(ROOT).as_posix().replace("/", "\\\\")
        resources.append(f'{number} RCDATA "{relative}"')
    header.append("};")
current = sha256((ROOT / "dist/dinput8.dll").read_bytes()).hexdigest()
header.append("static constexpr const char* knownProxies[]={")
header.extend(f'    "{value}",' for value in dict.fromkeys((current, *previous_proxies)))
header.append("};")
(ROOT / "setup/Payload.h").write_text("\n".join(header) + "\n", encoding="ascii")
(ROOT / "setup/Setup.rc").write_text("\n".join(resources) + "\n", encoding="ascii")
print("Prepared Steam and GOG installer resources.")
