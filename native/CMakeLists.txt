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
    src/instance_lock.cpp
    src/instance_lock.h
    src/tray.cpp
    src/tray.h
    src/config_store.cpp
    src/config_store.h
    src/startup_registration.cpp
    src/startup_registration.h
    src/settings_dialog.cpp
    src/settings_dialog.h
    src/resource.h
    src/app.rc
)

target_compile_features(AutoMouseCursorHiderNative PRIVATE cxx_std_17)
target_compile_definitions(AutoMouseCursorHiderNative PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(AutoMouseCursorHiderNative PRIVATE /W4 /permissive- /MT$<$<CONFIG:Debug>:d>)
target_link_options(AutoMouseCursorHiderNative PRIVATE /SUBSYSTEM:WINDOWS)
target_link_libraries(AutoMouseCursorHiderNative PRIVATE Shell32 Comctl32)
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

add_executable(ConfigStoreTests
    tests/config_store_tests.cpp
    src/config_store.cpp
    src/config_store.h
)
target_compile_features(ConfigStoreTests PRIVATE cxx_std_17)
target_compile_definitions(ConfigStoreTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(ConfigStoreTests PRIVATE /W4 /permissive-)
target_link_libraries(ConfigStoreTests PRIVATE Shell32)
