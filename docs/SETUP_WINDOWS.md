# Windows setup and troubleshooting

## First run in Visual Studio 2026

1. In Visual Studio Installer, install **Desktop development with C++**, including the **MSVC x64/x86 build tools**, **C++ CMake tools for Windows**, and a **Windows SDK**. The GPU driver must support OpenGL **3.3 core**.
2. Extract the revised ZIP into a **new folder**, for example `C:\CAP4052\DF5Pool`. Open the inner folder that directly contains `CMakeLists.txt` and `CMakePresets.json`.
3. In **Visual Studio 2026**, choose **File > Open > Folder** and select that folder.
4. Select **`windows-x64-debug`** if Visual Studio asks for a configuration. Wait for CMake configuration to finish.
5. Select **`CAP4052_DF5_Pool.exe`** in the startup dropdown if it is not already selected, then press **F5**. Visual Studio builds the application before launching it.

**No PowerShell setup, Git, vcpkg, manual GLFW/GLM installation, or internet download is required for the project libraries in this complete ZIP.**

CMake verifies and extracts the bundled GLFW 3.4 and GLM 1.0.1 source archives, then builds the required libraries in the selected build directory. GLAD is included as source. GLFW and the MSVC C/C++ runtime are linked statically, so students do not need to copy a GLFW DLL beside the game. Windows system graphics libraries and the GPU driver remain operating-system components.

CMake does not install Visual Studio, a Windows SDK, or a GPU driver. Those prerequisites must already be present. Preserve `third_party/sources`: the normal first configuration uses these local archives. If they are absent, CMake can retrieve the pinned archives from their official HTTPS locations and verify their hashes, which requires internet access.

## Replacing an older package

Extract this revision into a fresh folder. The earlier setup generated a machine-specific **`CMakeUserPresets.json`** and vcpkg build caches. Do not copy those generated files into this revision. The revised ZIP already contains portable Windows presets, and the optional setup helper no longer generates user presets.

If VS still shows an old configuration, confirm the folder path and use its **Delete Cache and Reconfigure** command for the current project. Keep the original source files.

## Default game build and optional tests

The Windows game presets set `BUILD_TESTING=OFF`, leaving the pool application as the only project executable in the normal startup list. This avoids accidentally launching the console test runner while trying to play the game. It does not bypass Windows executable trust checks.

The completed physics tests remain in `tests/physics_tests.cpp`. To build and run them separately, open a VS developer terminal in the project folder and use:

```powershell
cmake --preset physics-only
cmake --build --preset physics-only
ctest --preset physics-only --output-on-failure
```

This configuration does not need graphics libraries and does not open a window. To include the tests in a game build, configure that build with `-DBUILD_TESTING=ON`.

## Camera and collision sounds

Hold **C** to orbit around the table center and hold **Shift+C** to reverse direction. Release C to stop at the current angle. The camera keeps the same 4.4 m horizontal radius and 40-degree downward viewing angle. It can orbit while the physics is paused, and its movement does not change ball positions or velocities.

Ball and cushion collisions produce synthesized impact sounds. Greater incoming speed along the contact normal produces a louder sound. **M** toggles mute. The audio code is included with the project and does not require a sound-file download or separate audio SDK. If no playback device is available, the game remains playable without sound.

## Snapdragon / Windows ARM64

Use `windows-x64-debug` when the x64 compiler tools and the machine's x64 graphics support are available. Windows ARM can run that x64 application through emulation, subject to driver support.

For a native ARM64 build, install the **ARM64 C++ build tools** in Visual Studio Installer, then select **`windows-arm64-debug`**. CMake builds GLFW and the application for the selected architecture. The output is `b\arm64-debug\CAP4052_DF5_Pool.exe`.

An ARM64 build requires a compatible ARM64 OpenGL driver. A compiler installation cannot supply a missing graphics-driver capability. The presets use separate build directories for x64 and ARM64.

## Optional command-line helper

Students using the Visual Studio workflow above do not need this script. From a regular PowerShell window in the project folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup.ps1
```

The helper finds an installed VS 2026 compiler, initializes its developer environment, uses that installation's CMake/Ninja, and configures/builds the committed `windows-x64-debug` preset. It launches the game after a successful build. It does not install Git or vcpkg, write `CMakeUserPresets.json`, or change Windows security settings. The execution-policy switch applies only to that PowerShell process.

Add **`-NoRun`** to build without launching. Add **`-Architecture arm64`** to use the ARM64 preset, after installing its target compiler. To launch an already built application later, `scripts/run.ps1` accepts the same architecture option.

From an appropriate VS developer terminal, the equivalent commands are:

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug --parallel 4
.\b\x64-debug\CAP4052_DF5_Pool.exe
```

## Smart App Control

Dependency preparation and executable trust are separate issues. CMake does not disable Smart App Control, Microsoft Defender, or any Windows application-control policy. Windows can still block a locally compiled unsigned executable. Changing a target or skipping tests does not guarantee that the game will be trusted.

For the same C++ source running through WSL 2/WSLg while leaving Smart App Control enabled, see [RUN_WITH_SMART_APP_CONTROL_ON.md](RUN_WITH_SMART_APP_CONTROL_ON.md). That is a Linux build running on Windows. Native Windows execution under enforced signing requirements needs an appropriate trusted code-signing workflow.

## Common problems

| Symptom | Action |
|---|---|
| No C++ compiler or Windows SDK | Add the required C++ workload/tools in Visual Studio Installer. |
| No CMake/Ninja support | Add **C++ CMake tools for Windows**. |
| No pool startup target | Wait for configuration, read the first CMake error, and confirm `windows-x64-debug`. |
| Only `pool_physics_tests` appears | Select the Windows game preset in a fresh extraction of the revised package. |
| Missing source archive or failed fallback download | Restore `third_party/sources` from the complete ZIP, or check the internet/proxy connection before reconfiguring. Do not disable hash/TLS verification. |
| Duplicate preset or old vcpkg path | Use a fresh extraction without an old `CMakeUserPresets.json` or build cache. |
| Architecture changes do not take effect | Select the appropriate supplied preset and its separate build directory. |
| OpenGL context creation fails | Check the GPU driver and its OpenGL 3.3 support. |
| Windows application-control block | Preserve the blocked filename and policy message. See the Smart App Control section. |
| Game opens but does not shoot | A new shot requires stopped balls. Press P to resume if paused. |
| Camera moves only while C is held | This is the intended orbit control. Hold Shift+C for the reverse direction. |
| No collision sound | Check M mute state, the system playback volume/output, and any audio-device message in the console. The game continues if audio is unavailable. |

The project is validated on Linux during preparation. The VS 2026 folder-open workflow and optional PowerShell helper have not been executed on a Windows machine here.

## Official references

- [Microsoft: CMake projects in Visual Studio](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-180)
- [Microsoft: Configure and build with CMake Presets](https://learn.microsoft.com/en-us/cpp/build/cmake-presets-vs)
- [CMake: FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [GLFW: Building applications](https://www.glfw.org/docs/3.4/build.html)
- [Microsoft: Code signing for Smart App Control](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)
