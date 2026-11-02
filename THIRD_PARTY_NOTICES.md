# Third-party components

## GLAD loader (included in this ZIP)

- Upstream: https://github.com/Dav1dde/glad
- Pinned generator: **v0.1.36**, commit `1ecd45775d96f35170458e6b148eb0708967e402`
- Generated for OpenGL **3.3 core**, no optional extensions, using the pinned generator's bundled specification and `--reproducible`.
- License notices: `third_party/glad/LICENSE.txt` and the notices inside the generated headers.
- The generator source is MIT licensed. Khronos specifications/headers carry their upstream Apache/permissive notices.
- Included files: `include/glad/glad.h`, `include/KHR/khrplatform.h`, and `src/glad.c`.

## GLFW 3.4 and GLM 1.0.1 (source archives included)

CMake verifies and extracts the unmodified archives in `third_party/sources/`, then builds GLFW statically and uses GLM headers. No Git checkout, vcpkg installation, or global package installation is needed for the Windows build. If an archive is missing, the same verified upstream archive can be downloaded automatically.

- GLFW 3.4 upstream: https://www.glfw.org / https://github.com/glfw/glfw
- Official archive: https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
- GLFW archive SHA-256: `c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01`
- GLFW license: zlib/libpng, included in the archive and separately as `third_party/sources/GLFW-LICENSE.md`.
- GLM 1.0.1 upstream: https://github.com/g-truc/glm
- Official archive: https://codeload.github.com/g-truc/glm/tar.gz/refs/tags/1.0.1
- GLM archive SHA-256: `9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c`
- GLM provides the Happy Bunny License or MIT License. The full upstream license text is included in the archive and separately as `third_party/sources/GLM-copying.txt`.

No LearnOpenGL tutorial code is included. OpenGL itself is supplied by the operating system and graphics driver.

## miniaudio 0.11.25 (included header)

- Upstream: https://github.com/mackron/miniaudio
- Included file: `third_party/miniaudio/miniaudio.h`.
- License: upstream choice of public-domain/Unlicense or MIT No Attribution, reproduced in `third_party/miniaudio/LICENSE`.
- The exact source URL and verified SHA-256 are in `third_party/miniaudio/PROVENANCE.md`.
- The application uses the playback-device API for collision clicks synthesized by `src/CollisionAudio.h`. There are no downloaded or externally licensed sound recordings.
