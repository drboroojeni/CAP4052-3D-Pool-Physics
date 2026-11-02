#!/usr/bin/env bash
# Build the same C++ sources as Linux executables and run through WSLg.
# This script never installs packages or changes Windows security settings.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: bash scripts/run-wsl.sh [--build-only] [--software-rendering] [-- APP_ARGS...]
  --build-only          Configure, build, and run the physics tests; do not open a window.
  --software-rendering  Use Mesa software rendering for the application process.
  --                    Pass all remaining arguments to CAP4052_DF5_Pool.
  --help                Show this message.
EOF
}

build_only=false
software_rendering=false
app_args=()
while (($#)); do
    case "$1" in
        --build-only) build_only=true; shift ;;
        --software-rendering) software_rendering=true; shift ;;
        --) shift; app_args=("$@"); break ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'Unknown helper option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ "$(uname -s)" != Linux ]]; then
    echo 'Run this helper inside the Ubuntu / WSL terminal, not Windows PowerShell.' >&2
    exit 1
fi

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$project_root/b/wsl-debug"

missing=()
for tool in cmake ninja c++ pkg-config; do
    command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done
if command -v pkg-config >/dev/null 2>&1; then
    pkg-config --exists glfw3 glm gl || missing+=('GLFW/GLM/OpenGL development packages')
fi
if ((${#missing[@]})); then
    printf 'Missing dependencies: %s\n' "${missing[*]}" >&2
    cat >&2 <<'EOF'
Install dependencies manually inside Ubuntu, then rerun this helper:
sudo apt update && sudo apt install -y build-essential cmake ninja-build pkg-config libglfw3-dev libglm-dev libgl1-mesa-dev libgl1-mesa-dri mesa-utils unzip gdb rsync zip
EOF
    exit 1
fi

case "$project_root" in
    /mnt/[a-zA-Z]/*)
        echo 'Tip: extract the project into your Linux home folder for faster builds and separate Linux caches.' >&2
        ;;
esac

printf 'Project: %s\nBuild:   %s\n' "$project_root" "$build_dir"
# Direct -S/-B configuration intentionally avoids Windows CMakeUserPresets.json.
# An empty toolchain disables any inherited Windows/vcpkg toolchain environment.
cmake -S "$project_root" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE= \
    -DBUILD_RENDERER=ON -DBUILD_TESTING=ON -DPOOL_USE_SYSTEM_DEPS=ON
cmake --build "$build_dir" --parallel 4
ctest --test-dir "$build_dir" --output-on-failure

if [[ "$build_only" == true ]]; then
    echo 'Linux application and physics tests built successfully; launch skipped (--build-only).'
    exit 0
fi

if [[ -z "${DISPLAY:-}" ]]; then
    cat >&2 <<'EOF'
Build and physics tests succeeded, but no X11 display is available for this GLFW build.
Open an Ubuntu terminal under WSL 2 with WSLg, then rerun this helper.
In Windows PowerShell, use `wsl -l -v` to check that Ubuntu is version 2.
See docs/RUN_WITH_SMART_APP_CONTROL_ON.md for WSLg and graphics-driver checks.
Do not manually replace DISPLAY with an arbitrary address.
EOF
    exit 1
fi
if grep -qi microsoft /proc/sys/kernel/osrelease && [[ ! -d /mnt/wslg ]]; then
    echo 'WSL was detected, but /mnt/wslg is absent. WSL 2 GUI support must be available to open the pool window.' >&2
    echo 'See docs/RUN_WITH_SMART_APP_CONTROL_ON.md; --build-only remains available for compilation/testing.' >&2
    exit 1
fi

cd -- "$project_root"
if [[ "$software_rendering" == true ]]; then
    echo 'Launching the Linux app with Mesa software rendering (this process only).'
    exec env LIBGL_ALWAYS_SOFTWARE=1 "$build_dir/CAP4052_DF5_Pool" "${app_args[@]}"
fi
echo 'Launching the Linux OpenGL app. Windows Smart App Control settings remain unchanged.'
exec "$build_dir/CAP4052_DF5_Pool" "${app_args[@]}"
