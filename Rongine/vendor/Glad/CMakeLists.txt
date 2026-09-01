add_library(Glad STATIC
  src/glad.c
  include/glad/glad.h
  include/KHR/khrplatform.h
)

target_include_directories(Glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(Glad PRIVATE c_std_11)
