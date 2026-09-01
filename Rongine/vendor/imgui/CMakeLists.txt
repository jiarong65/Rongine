add_library(ImGui STATIC
  imgui.cpp
  imgui_draw.cpp
  imgui_widgets.cpp
  imgui_demo.cpp
  imconfig.h
  imgui.h
  imgui_internal.h
  imstb_rectpack.h
  imstb_textedit.h
  imstb_truetype.h
)

target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ImGui PRIVATE cxx_std_17)
