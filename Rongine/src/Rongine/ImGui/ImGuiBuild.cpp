#include "Rongpch.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

// Core ImGui is built as vendor ImGui.lib (linked by Rongine). This TU only pulls in backends.
#include "examples/imgui_impl_opengl3.cpp"
#include "examples/imgui_impl_glfw.cpp"