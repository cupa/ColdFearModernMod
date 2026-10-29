# Build

Requires Windows, Visual Studio 2022 or later with MSVC x86 and a Windows SDK.
Python 3 is required for packaging. No game files are needed to build or test.

```bat
Build.cmd
Test.cmd
BuildInstaller.cmd
python Package.py
```

Scripts select the x86 toolchain automatically. Set `MOD_PYTHON` if Python is not
on PATH. Runtime files go to `dist/`, tests to `build/` and downloads to `packages/`.
The installer build also tests removal, backups, rollback and save preservation.

`src/` contains runtime hooks, controller input, display support and tests.
`setup/` contains the installer, uninstaller and resource generation.
`config/` holds defaults; `scripts/` holds launch tools.

Hooks target the executable hash in README.md. Keep compatibility checks intact.
A 64-bit build is unsupported. When changing versions, update build scripts,
package names, source banners, manifests and documentation together.
