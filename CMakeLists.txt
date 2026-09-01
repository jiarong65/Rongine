cmake_minimum_required(VERSION 3.28)

project(Rongine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Use static MSVC runtime (/MT /MTd) to match the original premake staticruntime "On".
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
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)

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
    _CRT_SECURE_NO_WARNINGS
    YAML_CPP_STATIC_DEFINE
    RONG_PLATFORM_WINDOWS
    GLFW_INCLUDE_NONE
    _UNICODE
    UNICODE
  )

  if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${target} PRIVATE /utf-8 /EHsc)
  else()
    target_compile_options(${target} PRIVATE -finput-charset=UTF-8 -fexceptions)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target} PRIVATE RONG_DEBUG)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
      target_compile_options(${target} PRIVATE /Od /Zi /RTC1)
    else()
      target_compile_options(${target} PRIVATE -O0 -g)
    endif()
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${target} PRIVATE RONG_RELEASE)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
      target_compile_options(${target} PRIVATE /O2 /Zi)
    else()
      target_compile_options(${target} PRIVATE -O2 -g)
    endif()
  elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
    target_compile_definitions(${target} PRIVATE RONG_DIST)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
      target_compile_options(${target} PRIVATE /O2)
    else()
      target_compile_options(${target} PRIVATE -O2)
    endif()
  endif()
endfunction()

# ---------------------------------------------------------------------------
# OpenCASCADE (OCCT) libraries used by Rongine core and all consumers
# ---------------------------------------------------------------------------
set(OCCT_LIBS
  TKernel TKMath TKG3d TKBRep TKPrim TKMesh TKTopAlgo TKBO
  TKGeomAlgo TKGeomBase TKFillet TKOffset TKSTEP TKIGES TKShHealing
  TKXSBase TKSTEPBase TKSTEPAttr TKSTEP209 TKCAF
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/ImGuizmo
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/yaml-cpp/include
)

target_link_directories(Rongine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/win64/vc14/lib
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/inc
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/ImGuizmo
)

target_link_directories(Rongine-Editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/win64/vc14/lib
)

set(OCCT_LIBS
  TKernel TKMath TKG3d TKBRep TKPrim TKMesh TKTopAlgo TKBO
  TKGeomAlgo TKGeomBase TKFillet TKOffset TKSTEP TKIGES TKShHealing
  TKXSBase TKSTEPBase TKSTEPAttr TKSTEP209 TKCAF
)
target_link_libraries(Rongine-Editor PRIVATE Rongine ${OCCT_LIBS})

file(GLOB OCCT_DLLS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Rongine/vendor/OCCT/win64/vc14/bin/*.dll"
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
