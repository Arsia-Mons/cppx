cmake_minimum_required(VERSION 3.28)

project(sdl3_clay_hello LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "Build SDL static library" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.30
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

find_package(SDL3_ttf CONFIG QUIET)
if(NOT SDL3_ttf_FOUND)
  set(SDLTTF_VENDORED ON CACHE BOOL "Use vendored deps for SDL_ttf" FORCE)
  set(SDLTTF_SAMPLES OFF CACHE BOOL "Build SDL_ttf samples" FORCE)
  set(SDLTTF_INSTALL OFF CACHE BOOL "Install SDL_ttf" FORCE)
  FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3_ttf)
endif()

find_package(CURL REQUIRED)

enable_testing()

add_executable(hello
  src/main.cpp
  src/react.cpp
  src/ui/focus/ui_focus.cpp
  src/ui/components/app.cpp
  src/ui/components/counter.cpp
  src/ui/components/image.cpp
  src/ui/providers/input_provider.cpp
  src/ui/providers/theme_provider.cpp
  third_party/clay_impl.cpp
  third_party/clay/clay_renderer_SDL3.c
  third_party/stb_image_impl.cpp
)

target_include_directories(hello PRIVATE
  src
  third_party
  third_party/clay
)

# Clay ships clay_renderer_SDL3.c as C; compile it as C++ to keep one TU rule.
set_source_files_properties(third_party/clay/clay_renderer_SDL3.c PROPERTIES LANGUAGE CXX)

target_compile_options(hello PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(hello PRIVATE
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
  CURL::libcurl
)

add_executable(react_runtime_tests
  tests/react_runtime_tests.cpp
  src/react.cpp
  third_party/clay_impl.cpp
)

target_include_directories(react_runtime_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(react_runtime_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME react_runtime_tests COMMAND react_runtime_tests)

add_executable(ui_focus_tests
  tests/ui_focus_tests.cpp
  src/ui/focus/ui_focus.cpp
  third_party/clay_impl.cpp
)

target_include_directories(ui_focus_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(ui_focus_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_focus_tests COMMAND ui_focus_tests)
