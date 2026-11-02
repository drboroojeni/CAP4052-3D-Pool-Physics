# GLAD loader provenance

The included loader was generated from the official GLAD 1 generator, not copied from a tutorial project.

- Upstream: https://github.com/Dav1dde/glad
- Version: v0.1.36
- Verified tag commit: `1ecd45775d96f35170458e6b148eb0708967e402`
- Generation command: `python -m glad --profile core --api gl=3.3 --generator c --spec gl --extensions '' --out-path <this-directory> --reproducible`
- The `--reproducible` option uses the specification bundled at the pinned generator version.
- API: OpenGL 3.3 core, no optional extensions; `gladLoadGLLoader` entry point.
- Included notices: `LICENSE.txt` and the notices retained in generated headers.

GLAD's source is MIT licensed. Khronos specifications and headers have their own notices, reproduced in `LICENSE.txt`. GLFW and GLM are restored separately by vcpkg and retain their own licenses in the installed package share directories.
