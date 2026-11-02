# CAP4052 DF #5: completed 3D pool demonstration

This project is a completed, readable C++17/OpenGL/GLFW example for Discussion Forum #5. It shows how input, a fixed physics update, collision response, gameplay state, and 3D rendering fit together. The files contain the implementation, including the provided physics tests. Read the discussion prompt for what to submit.

## Student start: open the folder in Visual Studio 2026

1. Have **Visual Studio 2026** installed with **Desktop development with C++**, **MSVC x64/x86 tools**, **C++ CMake tools for Windows**, and a **Windows SDK**. The GPU driver must support OpenGL **3.3 core**.
2. Extract the revised ZIP into a **new, short folder**, such as `C:\CAP4052\DF5Pool`. Open the inner folder containing `CMakeLists.txt` and `CMakePresets.json` using **File > Open > Folder** in VS2026.
3. Select **`windows-x64-debug`** if asked and wait for CMake to finish configuring. CMake verifies/extracts the included library archives and prepares their build automatically.
4. Select **`CAP4052_DF5_Pool.exe`** in the startup dropdown if necessary, then press **F5** to build and run.

**The normal workflow needs no PowerShell setup, Git, vcpkg, manual library installation, or internet download for the project libraries.** GLFW 3.4 and GLM 1.0.1 are bundled as pinned source archives, and GLAD is included as source. CMake builds the required libraries locally. Preserve `third_party/sources`; official HTTPS downloads are available as a fallback if those archives are missing.

The Windows game preset builds the game without the optional console test runner, helping students select the intended application. The physics tests remain available separately. GLFW and the MSVC runtime link statically, so there is no separate GLFW DLL to copy.

**Updating an older copy:** extract this ZIP into a fresh folder. Do not copy an earlier `CMakeUserPresets.json`, `.vs`, or build cache into it. The revised ZIP already supplies the Windows presets.

See [docs/SETUP_WINDOWS.md](docs/SETUP_WINDOWS.md) for ARM64, optional tests, and troubleshooting. `scripts/setup.ps1` remains an optional command-line convenience; students using the folder-open workflow do not need it.

## Windows with Smart App Control enabled

CMake does not change Windows security settings or make an unsigned executable trusted. If Windows blocks a locally built executable, the same C++ source can instead run in Ubuntu on WSL 2, with its window displayed through WSLg. Follow [docs/RUN_WITH_SMART_APP_CONTROL_ON.md](docs/RUN_WITH_SMART_APP_CONTROL_ON.md). This is a Linux runtime option, and WSL/Ubuntu setup is separate from the native VS workflow.

## Controls

| Control | Action |
|---|---|
| Mouse over the bed | Aim the cue toward the mouse's intersection with the table plane |
| Hold/release left mouse button | Prepare power, then strike when the balls are at rest |
| A / D | Adjust aim with the keyboard |
| W / S | Adjust the selected shot power |
| Space | Strike with the selected power |
| Hold C | Orbit the camera around the table center |
| Hold Shift + C | Orbit in the reverse direction |
| M | Mute/unmute collision sounds |
| P | Pause/resume simulation |
| N | Advance one physics tick while paused |
| R | Restore the rack |
| 1 / 2 / 3 / 4 | Load head-on, cushion, pocket, and scratch scenarios |
| F1 | Toggle the extended controls/help |
| F2 | Toggle CSV tracing |
| F3 | Switch rolling deceleration between 0.22 and 0.44 m/s² |
| L | Run the controlled free-roll test at 0.50 m/s |
| Esc | Exit |

Watch the on-screen controls and status while playing. Hold C to move the camera continuously around the table center, or hold Shift+C to reverse its direction. Release C to keep the current view. The camera stays at a horizontal radius of 4.4 m, looking down at 40 degrees from the horizontal. Its rotation axis is the world y axis, perpendicular to the x-z table bed. Camera rotation also works while physics is paused and does not rotate the stored ball positions or velocities. A shot is accepted only when the simulation permits a new turn.

Ball-to-ball and ball-to-cushion impacts produce short, synthesized sounds. Louder impacts correspond to greater incoming speed along the contact normal; the sound uses a bounded volume range. This is relative approach speed for two balls, rather than the speed of just one ball. Press M to mute/unmute, or use your system's playback volume. If an audio device cannot be opened, the game continues without sound. No sound files or audio downloads are required at runtime.

## Files worth reading

| File | Responsibility |
|---|---|
| `src/main.cpp` | GLFW window, keyboard/mouse input, timing, state transitions, and the application loop |
| `src/Physics.h` | Ball state, vectors, physical parameters, and the public simulation interface |
| `src/Physics.cpp` | Motion updates, rolling slowdown, ball/ball and cushion responses, pocketing, and resets |
| `src/Renderer3D.h` / `.cpp` | Procedural meshes, shaders, camera matrices, table/cue/ball drawing, and feedback |
| `src/OrbitCamera.h` | Camera orbit geometry and angle updates, separate from ball motion |
| `src/CollisionAudio.h` / `.cpp` | Collision sound synthesis, impact-volume mapping, and playback |
| `tests/physics_tests.cpp` | Deterministic tests that run without opening a window |
| `CMakeLists.txt` | Application, physics library, GLAD loader, dependency links, and CTest target |
| `CMakePresets.json` | Portable Windows game, Linux, and physics-only configurations |
| `third_party/sources/` | Verified GLFW/GLM source archives for automatic local dependency builds |
| `third_party/glad/` | Included OpenGL 3.3 core loader and its upstream notices |

All scene geometry is procedural. There are no required model, texture, font, or sound downloads at application runtime. Physics updates the ball state; rendering reads that state and builds the visible scene. Audio consumes collision events as feedback and does not change the physical response.

## What the model assumes

- Ball centers move in the table's horizontal **x-z plane**. **y** is the up axis used to draw the 3D scene.
- Time is measured in seconds and the table uses meter-scale world coordinates.
- The physics uses fixed ticks and bounded movement per substep. Rendering may run at a different rate.
- Ball contacts use circle overlap, separation, and an impulse along the contact normal. Cushions reverse/dampen the incoming normal component.
- Rolling resistance reduces speed until the ball sleeps. It does not reverse the direction.
- Pocketing changes a ball's active state. A cue-ball scratch uses a safe respot rather than leaving an overlapping ball in the rack.
- The model omits spin/English, detailed cloth deformation, airborne balls, and official match-rule scoring. Restitution and resistance are tuned teaching parameters. The 3D camera does not add an unmodeled vertical velocity to planar physics.

The substeps reduce missed contacts within the configured speed limits. They are not a general-purpose continuous collision solver for arbitrarily large velocities or arbitrary geometry.

## Experiments for the forum

Start with the unmodified project. Use the numbered scenarios, pause, single-step, and CSV tracing to collect evidence. Change one parameter at a time in your own copy, record the old/new values, predict the result, and compare it with the observation. Identify the responsible function and the state variables it changes.

For a controlled rolling-resistance comparison, press **L** and observe where the cue ball stops. Press **F3** to change the deceleration, then press **L** again from the same starting state. The launch speed remains 0.50 m/s in both runs. Press F2 before/after a run to save the tick-by-tick state as a `pool_trace_*.csv` file in the application's working directory. Its columns include tick, time, ball ID, x/z position, x/z velocity, pocket state, event counters, and deceleration.

The project demonstrates implementation patterns that transfer to your approved HW #3 project: input-to-action mapping, state updates, collisions, 3D models/view/projection, feedback, reset behavior, and testing. Follow your own approved game proposal and HW #3 specification when choosing which patterns to reuse. Credit the provided example and any external assistance in your submission.

## Linux and dependency-free tests

For a Debian/Ubuntu development machine with CMake, Ninja, GCC, GLFW, GLM, and OpenGL development packages:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
./b/linux-debug/CAP4052_DF5_Pool
```

To test physics without GLFW, GLM, or OpenGL:

```bash
cmake -S . -B b/physics -DBUILD_RENDERER=OFF -DBUILD_TESTING=ON
cmake --build b/physics
ctest --test-dir b/physics --output-on-failure
```

This second command sequence uses the platform's default generator and avoids a Ninja requirement. On Windows, run it from a VS developer terminal with CMake available.

## Validation

The full graphical C++ target was configured and compiled on Linux, and its CTest physics suite passed during preparation. The Windows setup scripts were reviewed but have not been executed on a Windows/Visual Studio 2026 machine. The Windows instructions identify the required compiler, SDK, and graphics-driver components.

## Course reference and HW #3 authorship

See [docs/AUTHORSHIP_AND_HW3_USE.md](docs/AUTHORSHIP_AND_HW3_USE.md). Study the completed implementation and credit it. Continue your approved game proposal and implement your own gameplay-specific work for HW #3.

## Dependency provenance

CMake verifies and extracts the included GLFW 3.4 and GLM 1.0.1 source archives. If an archive is absent, it can download the same pinned archive from its official upstream location and verify its hash. The included GLAD loader was generated reproducibly with official GLAD v0.1.36 for OpenGL 3.3 core. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [third_party/glad/PROVENANCE.md](third_party/glad/PROVENANCE.md).
