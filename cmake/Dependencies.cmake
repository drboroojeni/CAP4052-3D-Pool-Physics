# FetchContent unpacks the included archives and builds the library targets.
# No package installation, Git, or vcpkg is needed for the Windows build.
option(POOL_USE_SYSTEM_DEPS "Use already-installed GLFW and GLM packages" OFF)

if(POOL_USE_SYSTEM_DEPS)
  find_package(glfw3 CONFIG REQUIRED)
  find_package(glm CONFIG REQUIRED)
  message(STATUS "Pool dependencies: installed GLFW and GLM packages")
  return()
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "GLFW library type" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Skip GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Skip GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Skip GLFW docs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "No system GLFW installation" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "Use static MSVC runtime" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Use header-only GLM" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Skip GLM tests" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "No system GLM installation" FORCE)
if(UNIX AND NOT APPLE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Use the X11 backend for this demo" FORCE)
endif()

set(pool_archive_dir "${CMAKE_CURRENT_LIST_DIR}/../third_party/sources")
set(pool_glfw_source "${pool_archive_dir}/glfw-3.4.tar.gz")
set(pool_glm_source "${pool_archive_dir}/glm-1.0.1.tar.gz")
if(NOT EXISTS "${pool_glfw_source}")
  set(pool_glfw_source "https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4")
  message(STATUS "GLFW archive absent: downloading official release 3.4")
endif()
if(NOT EXISTS "${pool_glm_source}")
  set(pool_glm_source "https://codeload.github.com/g-truc/glm/tar.gz/refs/tags/1.0.1")
  message(STATUS "GLM archive absent: downloading official release 1.0.1")
endif()

FetchContent_Declare(glfw
  URL "${pool_glfw_source}"
  URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01
  TLS_VERIFY TRUE)
FetchContent_Declare(glm
  URL "${pool_glm_source}"
  URL_HASH SHA256=9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c
  TLS_VERIFY TRUE)

message(STATUS "Pool dependencies: preparing GLFW 3.4 and GLM 1.0.1 automatically")
FetchContent_MakeAvailable(glfw glm)
