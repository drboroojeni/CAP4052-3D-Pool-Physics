# Run the pool demo with Smart App Control left on

This route builds the **same C++/OpenGL/GLFW source code** as a Linux application inside Ubuntu on **WSL 2**, and displays its window on the Windows desktop through **WSLg**. It does not change Smart App Control, Microsoft Defender, certificate stores, or Windows allow lists.

This is an alternate runtime for the completed project. It does not change the trust classification of an existing native Windows/MSVC executable. A C++ logic change alone cannot guarantee that Windows will trust an unsigned executable. The native VS 2026 setup remains available in `SETUP_WINDOWS.md` for environments that permit those locally built executables.

## 1. Install or check Ubuntu on WSL 2

On a Windows 11 development computer without WSL, open **PowerShell as Administrator** and run:

```powershell
wsl --install -d Ubuntu
```

Restart Windows if requested. Open **Ubuntu** from the Start menu and complete its first-run Linux username/password setup. This password is used by `sudo` inside Ubuntu.

For an existing WSL installation, check it from Windows PowerShell:

```powershell
wsl -l -v
```

Ubuntu must show **VERSION 2**. WSL 1 does not support WSLg GUI applications. If an existing installation needs a WSL update, Microsoft's documented command is `wsl --update`; restart the WSL session afterward as directed by Windows. These installation/update commands change the WSL environment and may need administrator permission. The pool helper does not run them automatically.

## 2. Put the ZIP in an easy-to-find Windows folder

Place the revised package at:

```text
C:\CAP4052\CAP4052_DF5_3D_Pool_Completed.zip
```

Use the revised ZIP that contains `scripts/run-wsl.sh` and this guide. The old package does not contain the WSL helper. In Ubuntu, Windows drive C appears under `/mnt/c`.

## 3. Install Linux dependencies, extract into Linux home, and run

Run the following commands **inside the Ubuntu terminal**, not Windows PowerShell:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libglfw3-dev libglm-dev libgl1-mesa-dev libgl1-mesa-dri mesa-utils unzip gdb rsync zip

pool_wsl_dir="$HOME/CAP4052/Pool-WSL-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$pool_wsl_dir"
unzip /mnt/c/CAP4052/CAP4052_DF5_3D_Pool_Completed.zip -d "$pool_wsl_dir"
cd "$pool_wsl_dir/DF5Pool"
bash scripts/run-wsl.sh
```

The timestamp creates a new extraction folder so an earlier project copy is not overwritten. The project is built inside the Linux filesystem, rather than in `/mnt/c`; the Windows build cache and generated Windows user presets are not reused. Keep the folder path printed by the helper for later edits and reruns.

The helper checks dependencies, builds `CAP4052_DF5_Pool` and `pool_physics_tests` as Linux executables in `b/wsl-debug`, runs the physics tests, and opens the pool application. It does not download vcpkg or invoke Windows `.exe` build tools. You should see the same table, cue controls, physics, and continuous camera orbit as in the native build. Hold **C** to orbit, hold **Shift+C** to reverse, and release C to keep the current view. The camera keeps its focus on the center and its 40-degree downward angle, including while physics is paused. **F1** shows the controls.

Ball and cushion collisions also produce synthesized impact sounds, with greater incoming normal speed producing louder feedback. **M** toggles mute. If an audio output cannot be opened, the game continues without sound; preserve any audio-device message printed in the Ubuntu terminal when troubleshooting. No separate sound assets are required.

## Rebuild or test later

Return to the same extracted `DF5Pool` folder in Ubuntu and run:

```bash
bash scripts/run-wsl.sh
```

To build and run only the tests, without opening a GUI window:

```bash
bash scripts/run-wsl.sh --build-only
```

To request Mesa software rendering for this application process:

```bash
bash scripts/run-wsl.sh --software-rendering
```

To pass the application's optional command-line arguments, place them after `--`:

```bash
bash scripts/run-wsl.sh -- --camera 1 --scenario 2
```

`--camera 1` still selects an opposite-side starting angle for an automated capture. It does not restore the old two-position C toggle. To choose another starting azimuth in degrees, use, for example, `--camera-angle 90` after `--`; live orbit controls remain available during normal interactive play.

The helper never runs `sudo` or installs packages itself. Package installation above is an explicit, one-time user action. The helper uses a direct `cmake -S ... -B ...` invocation so a Windows `CMakeUserPresets.json` cannot redirect it to Windows toolchain paths.

## Graphics and environment checks

- Use **WSL 2 with WSLg**. The pool program needs a GUI display and an OpenGL **3.3 core** context.
- A normal WSLg Ubuntu session supplies `DISPLAY` and the `/mnt/wslg` directory. Do not substitute an arbitrary `DISPLAY` address. If these are missing, check WSL version/update status using Microsoft's WSLg guide.
- Run `glxinfo -B` in Ubuntu to see the OpenGL renderer/version. A context-creation failure can indicate a graphics-driver or WSLg problem. Keep the Windows GPU driver current for accelerated OpenGL support.
- `--software-rendering` can use Mesa's software renderer when acceleration is unavailable, provided WSLg still supplies the GUI display. It does not create GUI support for WSL 1.
- If the game does not open, preserve the Ubuntu terminal output. The helper stops if compilation or physics tests fail, and prints a specific message if no display is available.

The source code has been built, tested, and rendered on Linux during preparation. WSLg availability and OpenGL performance depend on the actual Windows/WSL/GPU environment; this package has not been executed on your computer. This route does not promise that an organization-managed device will permit installing WSL.

## Editing the project

The files remain ordinary C++ source files. Edit the **Linux project copy** and rerun the helper in Ubuntu to compile your changes. The Linux pool window is separate from Visual Studio's native Windows debugger; this guide does not launch `VsDebugConsole.exe` or claim to configure VS's remote Linux debugger. If you use Visual Studio for editing, keep track of which project copy you are editing so the Ubuntu build receives the changes.

## Official references

- [Microsoft: Install WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
- [Microsoft: Run Linux GUI apps with WSL](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)
- [Microsoft: Set up a WSL development environment and choose filesystem locations](https://learn.microsoft.com/en-us/windows/wsl/setup/environment)
- [Microsoft: Code signing for Smart App Control](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)
