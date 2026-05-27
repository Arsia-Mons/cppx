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
find_package(Python3 COMPONENTS Interpreter REQUIRED)

include(cmake/cppx_transpile.cmake)

FetchContent_Declare(
  yoga
  GIT_REPOSITORY https://github.com/facebook/yoga.git
  GIT_TAG v3.2.1
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR yoga
)
FetchContent_MakeAvailable(yoga)

enable_testing()

cppx_transpile(CPPX_FIXTURE_GENERATED
  tests/fixtures/cppx/panel.cppx
  tests/fixtures/cppx/components.hx
)
add_custom_target(cppx_generated_fixtures ALL DEPENDS ${CPPX_FIXTURE_GENERATED})

cppx_transpile(CPPX_RETAINED_COMPONENT_GENERATED
  tests/fixtures/retained_cppx/generated_tree.cppx
)

add_test(
  NAME cppx_transpiler_tests
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppx_transpiler_tests.py
    --tool ${CMAKE_CURRENT_SOURCE_DIR}/tools/cppx_transpile.py
    --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cppx
)
set_tests_properties(cppx_transpiler_tests PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(hello
  src/main.cpp
  src/react.cpp
  src/app/app.cpp
  src/app/game_loop.cpp
  src/client/ui/client_ui.cpp
  src/client/ui/components/hud_band.cpp
  src/client/ui/hooks/shooter_hud.cpp
  src/client/ui/hooks/shooter_weapons.cpp
  src/client/ui/providers/app_shell.cpp
  src/client/ui/providers/shooter_provider.cpp
  src/client/ui/navigation/screen_stack.cpp
  src/client/ui/ui_pipeline.cpp
  src/client/ui/screens/in_game/in_game_screen.cpp
  src/client/ui/screens/loadout/components/confirm_dialog.cpp
  src/client/ui/screens/loadout/components/equipment_slot.cpp
  src/client/ui/screens/loadout/components/weapon_tile.cpp
  src/client/ui/screens/loadout/loadout_screen.cpp
  src/client/ui/screens/loadout/loadout_state.cpp
  src/client/ui/screens/main_menu/main_menu_screen.cpp
  src/client/ui/screens/options/options_screen.cpp
  src/client/ui/screens/pause/pause_screen.cpp
  src/platform/control_mailbox.cpp
  src/platform/sdl/input.cpp
  src/platform/sdl/window.cpp
  src/renderer/font_registry.cpp
  src/renderer/sdl_retained_renderer.cpp
  src/game/inventory.cpp
  src/game/economy.cpp
  src/game/shooter_game.cpp
  src/ui/retained/components.cpp
  src/ui/retained/draw_list.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/focus.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
  third_party/stb_image_impl.cpp
)

target_include_directories(hello PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(hello PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(hello PRIVATE
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
  CURL::libcurl
  yogacore
)

if(WIN32)
  add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:hello>
      $<TARGET_FILE_DIR:hello>
    COMMAND_EXPAND_LISTS
  )
endif()

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

add_executable(retained_ui_runtime_tests
  tests/retained_ui_runtime_tests.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
)

target_include_directories(retained_ui_runtime_tests PRIVATE
  src
)

target_compile_options(retained_ui_runtime_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME retained_ui_runtime_tests COMMAND retained_ui_runtime_tests)

add_executable(retained_ui_yoga_tests
  tests/retained_ui_yoga_tests.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/yoga_flex_layout.cpp
)

target_include_directories(retained_ui_yoga_tests PRIVATE
  src
)

target_compile_options(retained_ui_yoga_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_yoga_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_yoga_tests COMMAND retained_ui_yoga_tests)

add_executable(retained_cppx_component_tests
  tests/retained_cppx_component_tests.cpp
  ${CPPX_RETAINED_COMPONENT_GENERATED}
  src/react.cpp
  src/ui/retained/components.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(retained_cppx_component_tests PRIVATE
  src
  ${CMAKE_CURRENT_SOURCE_DIR}
  third_party
  third_party/clay
)

target_compile_options(retained_cppx_component_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_cppx_component_tests PRIVATE
  yogacore
)

add_test(NAME retained_cppx_component_tests COMMAND retained_cppx_component_tests)

add_executable(retained_ui_primitives_tests
  tests/retained_ui_primitives_tests.cpp
  src/react.cpp
  src/ui/retained/components.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(retained_ui_primitives_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(retained_ui_primitives_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_primitives_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_primitives_tests COMMAND retained_ui_primitives_tests)

add_executable(retained_ui_draw_list_tests
  tests/retained_ui_draw_list_tests.cpp
  src/react.cpp
  src/ui/retained/components.cpp
  src/ui/retained/draw_list.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(retained_ui_draw_list_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(retained_ui_draw_list_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_draw_list_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_draw_list_tests COMMAND retained_ui_draw_list_tests)

add_executable(retained_ui_focus_tests
  tests/retained_ui_focus_tests.cpp
  src/react.cpp
  src/ui/retained/components.cpp
  src/ui/retained/focus.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(retained_ui_focus_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(retained_ui_focus_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_focus_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_focus_tests COMMAND retained_ui_focus_tests)

add_executable(ui_primitives_tests
  tests/ui_primitives_tests.cpp
  src/react.cpp
  src/ui/focus/ui_focus.cpp
  src/ui/primitives/button.cpp
  src/ui/primitives/focusable.cpp
  src/ui/primitives/selectable.cpp
  src/ui/primitives/toggle.cpp
  third_party/clay_impl.cpp
)

target_include_directories(ui_primitives_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(ui_primitives_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_primitives_tests COMMAND ui_primitives_tests)

add_executable(client_ui_tests
  tests/client_ui_tests.cpp
  src/react.cpp
  src/client/ui/client_ui.cpp
  src/client/ui/navigation/screen_stack.cpp
  src/ui/retained/components.cpp
  src/ui/retained/draw_list.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/focus.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(client_ui_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(client_ui_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(client_ui_tests PRIVATE
  yogacore
)

add_test(NAME client_ui_tests COMMAND client_ui_tests)

add_executable(ui_pipeline_tests
  tests/ui_pipeline_tests.cpp
  src/react.cpp
  src/client/ui/client_ui.cpp
  src/client/ui/navigation/screen_stack.cpp
  src/client/ui/ui_pipeline.cpp
  src/ui/retained/components.cpp
  src/ui/retained/draw_list.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/focus.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(ui_pipeline_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(ui_pipeline_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(ui_pipeline_tests PRIVATE
  yogacore
)

add_test(NAME ui_pipeline_tests COMMAND ui_pipeline_tests)

add_executable(shooter_ui_tests
  tests/shooter_ui_tests.cpp
  src/react.cpp
  src/client/ui/client_ui.cpp
  src/client/ui/components/hud_band.cpp
  src/client/ui/hooks/shooter_hud.cpp
  src/client/ui/hooks/shooter_weapons.cpp
  src/client/ui/providers/app_shell.cpp
  src/client/ui/providers/shooter_provider.cpp
  src/client/ui/navigation/screen_stack.cpp
  src/client/ui/ui_pipeline.cpp
  src/client/ui/screens/in_game/in_game_screen.cpp
  src/client/ui/screens/loadout/components/confirm_dialog.cpp
  src/client/ui/screens/loadout/components/equipment_slot.cpp
  src/client/ui/screens/loadout/components/weapon_tile.cpp
  src/client/ui/screens/loadout/loadout_screen.cpp
  src/client/ui/screens/loadout/loadout_state.cpp
  src/client/ui/screens/main_menu/main_menu_screen.cpp
  src/client/ui/screens/options/options_screen.cpp
  src/client/ui/screens/pause/pause_screen.cpp
  src/game/inventory.cpp
  src/game/economy.cpp
  src/game/shooter_game.cpp
  src/ui/retained/components.cpp
  src/ui/retained/draw_list.cpp
  src/ui/retained/flex_layout.cpp
  src/ui/retained/focus.cpp
  src/ui/retained/ui_tree.cpp
  src/ui/retained/yoga_flex_layout.cpp
  third_party/clay_impl.cpp
)

target_include_directories(shooter_ui_tests PRIVATE
  src
  third_party
  third_party/clay
)

target_compile_options(shooter_ui_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(shooter_ui_tests PRIVATE
  yogacore
)

add_test(NAME shooter_ui_tests COMMAND shooter_ui_tests)

add_test(
  NAME ui_cli_smoke
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_cli_smoke.py
    --exe $<TARGET_FILE:hello>
    --cli ${CMAKE_CURRENT_SOURCE_DIR}/tools/ui_cli.py
    --out ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_smoke.bmp
    --capture-dir ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_capture
    --control-dir ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_control
)

add_test(
  NAME ui_cli_dm
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_cli_dm.py
)

add_test(
  NAME ui_cli_commands
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_cli_commands.py
    --exe $<TARGET_FILE:hello>
    --cli ${CMAKE_CURRENT_SOURCE_DIR}/tools/ui_cli.py
    --control-dir ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_commands_control
    --out ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_commands.bmp
    --capture-dir ${CMAKE_CURRENT_BINARY_DIR}/ui_cli_commands_capture
)
