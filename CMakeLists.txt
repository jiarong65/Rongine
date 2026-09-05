cmake_minimum_required(VERSION 3.28)

project(Rongine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Static runtime (/MT /MTd) to match the original premake staticruntime "On".
# Despite the name, this also drives clang (GNU frontend) on Windows: CMake maps
# it to -D_MT + --dependent-lib=libcmtd/libcmt, so both toolchains stay static.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library selection" FORCE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Vendor libraries
# ---------------------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_MAIN_PROJECT OFF CACHE BOOL "" FORCE)

add_subdirectory(Rongine/vendor/GLFW)
add_subdirectory(Rongine/vendor/yaml-cpp)
add_subdirectory(Rongine/vendor/Glad)
add_subdirectory(Rongine/vendor/imgui)

if(TARGET glfw AND NOT TARGET GLFW)
  add_library(GLFW ALIAS glfw)
endif()

# ---------------------------------------------------------------------------
# Common helper
# ---------------------------------------------------------------------------
function(configure_rongine_target target)
  target_compile_definitions(${target} PRIVATE
    YAML_CPP_STATIC_DEFINE
    RONG_PLATFORM_WINDOWS
    GLFW_INCLUDE_NONE
    _UNICODE
    UNICODE
  )

  target_compile_options(${target} PRIVATE -finput-charset=UTF-8 -fexceptions)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target} PRIVATE RONG_DEBUG)
    target_compile_options(${target} PRIVATE -O0 -g)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${target} PRIVATE RONG_RELEASE)
    target_compile_options(${target} PRIVATE -O2 -g)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
    target_compile_definitions(${target} PRIVATE RONG_DIST)
    target_compile_options(${target} PRIVATE -O2)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# OpenCASCADE (OCCT) — resolve a prebuilt package.
# Priority: existing Rongine/vendor/OCCT (manual install, gitignored) >
# cached zip in build dir > official GitHub release download.
# OCCT 7.7.0 upstream only ships an NSIS installer, so fresh clones use 7.8.0.
# ---------------------------------------------------------------------------
set(OCCT_VERSION "7.8.0")
set(OCCT_RELEASE_TAG "V7_8_0")
set(OCCT_ARCHIVE_NAME "occt-vc143-64.zip")
set(OCCT_URL "https://github.com/Open-Cascade-SAS/OCCT/releases/download/${OCCT_RELEASE_TAG}/${OCCT_ARCHIVE_NAME}")

set(OCCT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT")
if(NOT EXISTS "${OCCT_ROOT}/inc/Standard_Version.hxx")
  set(OCCT_ARCHIVE "" CACHE FILEPATH "Pre-downloaded ${OCCT_ARCHIVE_NAME} for offline OCCT setup")
  set(_occt_zip "${OCCT_ARCHIVE}")
  if(NOT _occt_zip)
    set(_occt_zip "${CMAKE_BINARY_DIR}/${OCCT_ARCHIVE_NAME}")
  endif()
  if(NOT EXISTS "${_occt_zip}")
    message(STATUS "OCCT: downloading prebuilt ${OCCT_VERSION} (${OCCT_ARCHIVE_NAME}, ~250MB)...")
    file(DOWNLOAD "${OCCT_URL}" "${_occt_zip}" SHOW_PROGRESS STATUS _occt_dl TIMEOUT 900)
    list(GET _occt_dl 0 _occt_dl_code)
    if(NOT _occt_dl_code EQUAL 0)
      list(GET _occt_dl 1 _occt_dl_msg)
      file(REMOVE "${_occt_zip}")
      message(FATAL_ERROR
        "OCCT download failed (${_occt_dl_code} ${_occt_dl_msg}).\n"
        "Download ${OCCT_URL} manually, then re-run CMake with -DOCCT_ARCHIVE=<path-to-zip>.")
    endif()
  endif()

  set(_occt_extract "${CMAKE_BINARY_DIR}/occt-${OCCT_VERSION}")
  if(NOT EXISTS "${_occt_extract}")
    message(STATUS "OCCT: extracting ${_occt_zip}...")
    file(MAKE_DIRECTORY "${_occt_extract}")
    file(ARCHIVE_EXTRACT INPUT "${_occt_zip}" DESTINATION "${_occt_extract}")
  endif()

  # The archive may be flat or have a top-level occt-vc143-64/ folder.
  set(_occt_found "")
  set(_occt_candidates "${_occt_extract}")
  file(GLOB _occt_children LIST_DIRECTORIES TRUE "${_occt_extract}/*")
  list(APPEND _occt_candidates ${_occt_children})
  foreach(_r IN LISTS _occt_candidates)
    if(EXISTS "${_r}/inc/Standard_Version.hxx")
      set(_occt_found "${_r}")
      break()
    endif()
  endforeach()
  if(NOT _occt_found)
    message(FATAL_ERROR "OCCT: inc/Standard_Version.hxx not found after extracting ${_occt_zip}")
  endif()
  set(OCCT_ROOT "${_occt_found}")
  message(STATUS "OCCT: using downloaded prebuilt at ${OCCT_ROOT}")
else()
  message(STATUS "OCCT: using existing package at ${OCCT_ROOT}")
endif()

# Toolset folder may be named vc14 or vc143 depending on the package.
file(GLOB _occt_libdirs LIST_DIRECTORIES TRUE "${OCCT_ROOT}/win64/*/lib")
list(GET _occt_libdirs 0 OCCT_LIB_DIR)
file(GLOB _occt_bindirs LIST_DIRECTORIES TRUE "${OCCT_ROOT}/win64/*/bin")
list(GET _occt_bindirs 0 OCCT_BIN_DIR)

# Data-exchange toolkits were renamed in OCCT 7.8:
#   TKSTEP -> TKDESTEP (absorbing TKSTEPAttr/TKSTEP209), TKIGES -> TKDEIGES,
#   TKSTEPBase -> TKDECascade. TKXSBase keeps its name; TKDE is a new toolkit.
set(OCCT_DETECTED_VERSION "")
if(EXISTS "${OCCT_ROOT}/inc/Standard_Version.hxx")
  file(STRINGS "${OCCT_ROOT}/inc/Standard_Version.hxx" _occt_ver_def
       REGEX "^#define OCC_VERSION_COMPLETE[ \t]+\"[0-9.]+\"")
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" OCCT_DETECTED_VERSION "${_occt_ver_def}")
endif()
if(OCCT_DETECTED_VERSION AND OCCT_DETECTED_VERSION VERSION_LESS "7.8")
  set(OCCT_DEX_LIBS TKSTEP TKIGES TKXSBase TKSTEPBase TKSTEPAttr TKSTEP209)
else()
  set(OCCT_DEX_LIBS TKDESTEP TKDEIGES TKXSBase TKDECascade)
endif()

set(OCCT_LIBS
  TKernel TKMath TKG3d TKBRep TKPrim TKMesh TKTopAlgo TKBO
  TKGeomAlgo TKGeomBase TKFillet TKOffset TKShHealing TKCAF
  ${OCCT_DEX_LIBS}
)

# ---------------------------------------------------------------------------
# Rongine core library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE RONGINE_CPP CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/*.cpp"
)
file(GLOB_RECURSE RONGINE_HDR CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/*.h"
)
file(GLOB_RECURSE RONGINE_IX_ALL CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/*.ixx"
)
set(RONGINE_IX ${RONGINE_IX_ALL})
list(FILTER RONGINE_IX EXCLUDE REGEX ".*[\\/]Rongine\\.ApplicationImpl\\.ixx$")
list(APPEND RONGINE_CPP
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/stb_image/stb_image.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/ImGuizmo/ImGuizmo.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Core/Rongine.ApplicationImpl.ixx"
)
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Core/Rongine.ApplicationImpl.ixx"
  PROPERTIES LANGUAGE CXX
)

add_library(Rongine STATIC ${RONGINE_CPP} ${RONGINE_HDR})
target_sources(Rongine PUBLIC
  FILE_SET CXX_MODULES
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src
  FILES ${RONGINE_IX}
)

target_include_directories(Rongine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/spdlog/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Events
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Core
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/CAD
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Commands
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/ImGui
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Renderer
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Renderer/Threading
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Scene
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src/Rongine/Utils
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/GLFW/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/Glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/imgui/examples
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/stb_image
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/entt/include
  ${OCCT_ROOT}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/ImGuizmo
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/yaml-cpp/include
)

target_link_directories(Rongine PUBLIC
  ${OCCT_LIB_DIR}
)
target_link_libraries(Rongine PUBLIC
  GLFW
  Glad
  ImGui
  yaml-cpp
  opengl32.lib
  ${OCCT_LIBS}
)

target_compile_definitions(Rongine PRIVATE RONG_BUILD_DLL)
configure_rongine_target(Rongine)

# ---------------------------------------------------------------------------
# Sandbox
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SANDBOX_CPP CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/src/*.cpp"
)
file(GLOB_RECURSE SANDBOX_HDR CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/src/*.h"
)
file(GLOB_RECURSE SANDBOX_IX CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/src/*.ixx"
)

add_executable(Sandbox ${SANDBOX_CPP} ${SANDBOX_HDR})
target_sources(Sandbox PRIVATE
  FILE_SET CXX_MODULES
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/src
  FILES ${SANDBOX_IX}
)

target_include_directories(Sandbox PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/spdlog/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/Glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/entt/include
  ${OCCT_ROOT}/inc
)
target_link_libraries(Sandbox PRIVATE Rongine)
configure_rongine_target(Sandbox)

# Copy assets next to the executable so relative "assets/..." paths resolve at runtime
add_custom_command(TARGET Sandbox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/assets $<TARGET_FILE_DIR:Sandbox>/assets
  COMMENT "Copying sandbox assets next to Sandbox"
)

# ---------------------------------------------------------------------------
# Rongine-Editor
# ---------------------------------------------------------------------------
file(GLOB_RECURSE EDITOR_CPP CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine-Editor/src/*.cpp"
)
file(GLOB_RECURSE EDITOR_HDR CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine-Editor/src/*.h"
)
file(GLOB_RECURSE EDITOR_IX CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine-Editor/src/*.ixx"
)

add_executable(Rongine-Editor ${EDITOR_CPP} ${EDITOR_HDR})
target_sources(Rongine-Editor PRIVATE
  FILE_SET CXX_MODULES
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Rongine-Editor/src
  FILES ${EDITOR_IX}
)

target_include_directories(Rongine-Editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/spdlog/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/src
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/Glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/entt/include
  ${OCCT_ROOT}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/ImGuizmo
)

target_link_directories(Rongine-Editor PRIVATE
  ${OCCT_LIB_DIR}
)

target_link_libraries(Rongine-Editor PRIVATE Rongine ${OCCT_LIBS})

file(GLOB OCCT_DLLS CONFIGURE_DEPENDS
  "${OCCT_BIN_DIR}/*.dll"
)
add_custom_command(TARGET Rongine-Editor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OCCT_DLLS} $<TARGET_FILE_DIR:Rongine-Editor>
  COMMENT "Copying OCCT DLLs next to Rongine-Editor"
)

add_custom_command(TARGET Rongine-Editor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Rongine-Editor/assets $<TARGET_FILE_DIR:Rongine-Editor>/assets
  COMMENT "Copying editor assets next to Rongine-Editor"
)

configure_rongine_target(Rongine-Editor)
