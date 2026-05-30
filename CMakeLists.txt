cmake_minimum_required(VERSION 3.28)

project(sdl3_retained_ui LANGUAGES C CXX)

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

cppx_transpile(UI_COMPONENT_GENERATED
  src/ui/components/box.cppx
  src/ui/components/box.hx
  src/ui/components/button.cppx
  src/ui/components/button.hx
  src/ui/components/checkbox.cppx
  src/ui/components/checkbox.hx
  src/ui/components/dialog.cppx
  src/ui/components/dialog.hx
  src/ui/components/input.cppx
  src/ui/components/input.hx
  src/ui/components/text.cppx
  src/ui/components/text.hx
)
add_custom_target(cppx_generated_components DEPENDS ${UI_COMPONENT_GENERATED})

cppx_transpile(CLIENT_UI_GENERATED
  src/client/ui/components/screen_chrome.cppx
  src/client/ui/components/screen_chrome.hx
  src/client/ui/screens/in_game/in_game_screen.cppx
  src/client/ui/screens/in_game/in_game_screen.hx
  src/client/ui/screens/main_menu/main_menu_screen.cppx
  src/client/ui/screens/main_menu/main_menu_screen.hx
  src/client/ui/screens/options/options_screen.cppx
  src/client/ui/screens/options/options_screen.hx
  src/client/ui/screens/pause/pause_screen.cppx
  src/client/ui/screens/pause/pause_screen.hx
)
add_custom_target(cppx_generated_client_ui DEPENDS ${CLIENT_UI_GENERATED})

set(UI_COMPONENT_GENERATED_INCLUDE_DIRS
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cppx/src"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cppx/src/ui/components"
)

set(UI_COMPONENT_SOURCES
  ${UI_COMPONENT_GENERATED}
  src/ui/style/resolve.cpp
  src/ui/style/default_theme.cpp
  src/ui/style/text_measure.cpp
  src/ui/runtime/interaction_hooks.cpp
)

function(target_use_ui_components target_name)
  target_include_directories(${target_name} PRIVATE
    ${UI_COMPONENT_GENERATED_INCLUDE_DIRS}
  )
  add_dependencies(${target_name} cppx_generated_components)
endfunction()

cppx_transpile(CPPX_FIXTURE_GENERATED
  tests/fixtures/cppx/panel.cppx
  tests/fixtures/cppx/multiline.cppx
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

add_test(
  NAME cppx_editor_package_tests
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppx_editor_package_tests.py
)
set_tests_properties(cppx_editor_package_tests PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME runtime_dependency_guard
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime_dependency_guard.py
)
set_tests_properties(runtime_dependency_guard PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME ui_style_invariants_guard
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_style_invariants_guard.py
)
set_tests_properties(ui_style_invariants_guard PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME component_descriptor_guard
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/component_descriptor_guard.py
    --root ${CMAKE_CURRENT_SOURCE_DIR}
    --tool ${CMAKE_CURRENT_SOURCE_DIR}/tools/cppx_transpile.py
)
set_tests_properties(component_descriptor_guard PROPERTIES
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
  src/client/ui/screens/loadout/components/confirm_dialog.cpp
  src/client/ui/screens/loadout/components/equipment_slot.cpp
  src/client/ui/screens/loadout/components/weapon_tile.cpp
  src/client/ui/screens/loadout/loadout_screen.cpp
  src/client/ui/screens/loadout/loadout_state.cpp
  ${CLIENT_UI_GENERATED}
  src/platform/control_mailbox.cpp
  src/platform/sdl/input.cpp
  src/platform/sdl/window.cpp
  src/renderer/font_registry.cpp
  src/renderer/ui_surface.cpp
  src/renderer/draw_executor.cpp
  src/renderer/sdf_raster.cpp
  src/renderer/texture_registry.cpp
  src/renderer/text_measure_impl.cpp
  src/game/inventory.cpp
  src/game/economy.cpp
  src/game/shooter_game.cpp
  src/ui/runtime/draw_command.cpp
  src/ui/runtime/draw_command_builder.cpp
  src/ui/runtime/geometry.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/focus.cpp
  src/ui/runtime/tree.cpp
  src/ui/runtime/yoga_flex_layout.cpp
  third_party/stb_image_impl.cpp
)

target_include_directories(hello PRIVATE
  src
  third_party
)

target_compile_options(hello PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_dependencies(hello cppx_generated_client_ui)

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
)

target_include_directories(react_runtime_tests PRIVATE
  src
  third_party
)

target_compile_options(react_runtime_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME react_runtime_tests COMMAND react_runtime_tests)

add_executable(retained_ui_runtime_tests
  tests/retained_ui_runtime_tests.cpp
  src/ui/runtime/tree.cpp
  src/ui/style/text_measure.cpp
  src/ui/runtime/flex_layout.cpp
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
  src/ui/runtime/tree.cpp
  src/ui/style/text_measure.cpp
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/yoga_flex_layout.cpp
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

add_executable(retained_ui_reconciler_tests
  tests/retained_ui_reconciler_tests.cpp
  src/react.cpp
  src/ui/runtime/element.cpp
  src/ui/runtime/tree.cpp
  src/ui/style/text_measure.cpp
)

target_include_directories(retained_ui_reconciler_tests PRIVATE
  src
)

target_compile_options(retained_ui_reconciler_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME retained_ui_reconciler_tests COMMAND retained_ui_reconciler_tests)

add_executable(ui_components_tests
  tests/ui_components_tests.cpp
  src/react.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/tree.cpp
)

target_include_directories(ui_components_tests PRIVATE
  src
)

target_compile_options(ui_components_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_components_tests COMMAND ui_components_tests)

add_executable(retained_cppx_component_tests
  tests/retained_cppx_component_tests.cpp
  ${CPPX_RETAINED_COMPONENT_GENERATED}
  src/react.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/tree.cpp
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(retained_cppx_component_tests PRIVATE
  src
  ${CMAKE_CURRENT_SOURCE_DIR}
  third_party
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
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/tree.cpp
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(retained_ui_primitives_tests PRIVATE
  src
  third_party
)

target_compile_options(retained_ui_primitives_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_primitives_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_primitives_tests COMMAND retained_ui_primitives_tests)

add_executable(retained_ui_draw_command_tests
  tests/retained_ui_draw_command_tests.cpp
  src/react.cpp
  src/ui/runtime/draw_command_builder.cpp
  src/ui/runtime/draw_command.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/tree.cpp
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(retained_ui_draw_command_tests PRIVATE
  src
  third_party
)

target_compile_options(retained_ui_draw_command_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_draw_command_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_draw_command_tests COMMAND retained_ui_draw_command_tests)

# --- from-first-principles styling system (src/ui/style) ---
add_executable(ui_style_tests
  tests/ui_style_tests.cpp
  src/ui/style/default_theme.cpp
  src/ui/style/text_measure.cpp
)

target_include_directories(ui_style_tests PRIVATE
  src
  third_party
)

target_compile_options(ui_style_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_style_tests COMMAND ui_style_tests)

add_executable(ui_style_resolve_tests
  tests/ui_style_resolve_tests.cpp
  src/ui/style/resolve.cpp
)

target_include_directories(ui_style_resolve_tests PRIVATE
  src
  third_party
)

target_compile_options(ui_style_resolve_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_style_resolve_tests COMMAND ui_style_resolve_tests)

# --- SDL-free geometry tessellation math (src/ui/runtime/geometry) ---
add_executable(ui_geometry_tests
  tests/ui_geometry_tests.cpp
  src/ui/runtime/geometry.cpp
)

target_include_directories(ui_geometry_tests PRIVATE
  src
  third_party
)

target_compile_options(ui_geometry_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_geometry_tests COMMAND ui_geometry_tests)

add_executable(ui_interaction_hooks_tests
  tests/ui_interaction_hooks_tests.cpp
  src/ui/runtime/interaction_hooks.cpp
  src/react.cpp
)

target_include_directories(ui_interaction_hooks_tests PRIVATE
  src
  third_party
)

target_compile_options(ui_interaction_hooks_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_interaction_hooks_tests COMMAND ui_interaction_hooks_tests)

add_executable(ui_draw_command_tests
  tests/ui_draw_command_tests.cpp
  src/ui/runtime/draw_command.cpp
)

target_include_directories(ui_draw_command_tests PRIVATE
  src
  third_party
)

target_compile_options(ui_draw_command_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_test(NAME ui_draw_command_tests COMMAND ui_draw_command_tests)

# --- P3 renderer: new IR -> pixels, golden-verified (SDL) ---
add_executable(renderer_golden_tests
  tests/renderer_golden_tests.cpp
  src/renderer/draw_executor.cpp
  src/renderer/sdf_raster.cpp
  src/renderer/font_registry.cpp
  src/renderer/texture_registry.cpp
  src/ui/runtime/geometry.cpp
  src/ui/runtime/draw_command.cpp
)

target_include_directories(renderer_golden_tests PRIVATE
  src
  third_party
  tests
)

target_compile_options(renderer_golden_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(renderer_golden_tests PRIVATE
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

add_test(NAME renderer_golden_tests COMMAND renderer_golden_tests)
set_tests_properties(renderer_golden_tests PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(retained_ui_focus_tests
  tests/retained_ui_focus_tests.cpp
  src/react.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/focus.cpp
  src/ui/runtime/tree.cpp
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(retained_ui_focus_tests PRIVATE
  src
  third_party
)

target_compile_options(retained_ui_focus_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

target_link_libraries(retained_ui_focus_tests PRIVATE
  yogacore
)

add_test(NAME retained_ui_focus_tests COMMAND retained_ui_focus_tests)

add_executable(client_ui_tests
  tests/client_ui_tests.cpp
  src/react.cpp
  src/client/ui/client_ui.cpp
  src/client/ui/navigation/screen_stack.cpp
  src/ui/runtime/draw_command.cpp
  src/ui/runtime/draw_command_builder.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/focus.cpp
  src/ui/runtime/tree.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(client_ui_tests PRIVATE
  src
  third_party
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
  src/ui/runtime/draw_command.cpp
  src/ui/runtime/draw_command_builder.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/focus.cpp
  src/ui/runtime/tree.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(ui_pipeline_tests PRIVATE
  src
  third_party
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
  src/client/ui/screens/loadout/components/confirm_dialog.cpp
  src/client/ui/screens/loadout/components/equipment_slot.cpp
  src/client/ui/screens/loadout/components/weapon_tile.cpp
  src/client/ui/screens/loadout/loadout_screen.cpp
  src/client/ui/screens/loadout/loadout_state.cpp
  ${CLIENT_UI_GENERATED}
  src/game/inventory.cpp
  src/game/economy.cpp
  src/game/shooter_game.cpp
  src/ui/runtime/draw_command.cpp
  src/ui/runtime/draw_command_builder.cpp
  src/ui/runtime/element.cpp
  ${UI_COMPONENT_SOURCES}
  src/ui/runtime/flex_layout.cpp
  src/ui/runtime/focus.cpp
  src/ui/runtime/tree.cpp
  src/ui/runtime/yoga_flex_layout.cpp
)

target_include_directories(shooter_ui_tests PRIVATE
  src
  third_party
)

target_compile_options(shooter_ui_tests PRIVATE
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-missing-field-initializers -Wno-c99-designator -Wno-unused-parameter>
)

add_dependencies(shooter_ui_tests cppx_generated_client_ui)

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

foreach(ui_component_consumer IN ITEMS
  hello
  ui_components_tests
  retained_cppx_component_tests
  retained_ui_primitives_tests
  retained_ui_draw_command_tests
  retained_ui_focus_tests
  client_ui_tests
  ui_pipeline_tests
  shooter_ui_tests
)
  target_use_ui_components(${ui_component_consumer})
endforeach()
