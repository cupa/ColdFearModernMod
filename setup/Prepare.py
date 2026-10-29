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
    "ad897be3c24be9256b2bdfc6a01ae2fe0a00e4a5c2ff3237fa5b56e8c930e505",
    "f067bff4edbf882b62fbca0be9ad5b956b136f9585e10e7ef2c06fd1271890c7",
)
header = [
    "#pragma once",
    "struct PayloadFile {int id;const wchar_t* name;const char* hash;};",
    "static constexpr PayloadFile payload[]={",
]
resources = ['#include <windows.h>', '1 RT_MANIFEST "setup\\\\Setup.manifest"']
for number, name in enumerate(package.PLAYER_FILES, 101):
    if name == "Uninstall.exe" and "--uninstaller" in sys.argv:
        continue
    path = ROOT / "dist" / name
    if not path.is_file():
        raise FileNotFoundError(path)
    digest = sha256(path.read_bytes()).hexdigest()
    header.append(f'    {{{number},L"{name}","{digest}"}},')
    resources.append(f'{number} RCDATA "dist\\\\{name}"')
header.append("};")
current = sha256((ROOT / "dist/dinput8.dll").read_bytes()).hexdigest()
header.append("static constexpr const char* knownProxies[]={")
header.extend(f'    "{value}",' for value in dict.fromkeys((current, *previous_proxies)))
header.append("};")
(ROOT / "setup/Payload.h").write_text("\n".join(header) + "\n", encoding="ascii")
(ROOT / "setup/Setup.rc").write_text("\n".join(resources) + "\n", encoding="ascii")
print("Prepared verified installer resources.")
