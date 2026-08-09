cmake_minimum_required(VERSION 3.25)

project(AutoMouseCursorHiderNative LANGUAGES CXX RC)

add_executable(AutoMouseCursorHiderNative WIN32
    src/main.cpp
    src/cursor_manager.cpp
    src/cursor_manager.h
    src/cursor_state.cpp
    src/cursor_state.h
    src/mouse_monitor.cpp
    src/mouse_monitor.h
    src/resource.h
    src/app.rc
)

target_compile_features(AutoMouseCursorHiderNative PRIVATE cxx_std_17)
target_compile_definitions(AutoMouseCursorHiderNative PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(AutoMouseCursorHiderNative PRIVATE /W4 /permissive- /MT$<$<CONFIG:Debug>:d>)
target_link_options(AutoMouseCursorHiderNative PRIVATE /SUBSYSTEM:WINDOWS)
set_target_properties(AutoMouseCursorHiderNative PROPERTIES
    OUTPUT_NAME "AutoMouseCursorHider"
    VS_DPI_AWARE "PerMonitor"
)

add_executable(CursorStateTests
    tests/cursor_state_tests.cpp
    src/cursor_state.cpp
    src/cursor_state.h
)
target_compile_features(CursorStateTests PRIVATE cxx_std_17)
target_compile_definitions(CursorStateTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(CursorStateTests PRIVATE /W4 /permissive-)
