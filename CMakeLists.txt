cmake_minimum_required(VERSION 3.25)
project(RiverFlyerXbox LANGUAGES CXX)

# From-scratch engine for River Flyer on Xbox: no raylib, no third-party code.
# Targets:
#   rfengine   - the engine; Win32 + XInput on the PC (preset msvc), CoreWindow +
#                Windows.Gaming.Input for a Developer Mode Xbox (preset uwp).
#                Direct3D 11 and XAudio2 are the same source files on both.
#   rfcompat   - compat/raylib.h: the 86 raylib calls the game makes, on rfengine
#   demo       - phase 2 of the README plan: window, controller, a plane on a canvas
#   riverflyer - the unchanged River Flyer sources compiled against rfcompat

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT MSVC)
    message(FATAL_ERROR "This project targets Windows and Xbox with MSVC only.")
endif()

set(RF_UWP OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(RF_UWP ON)
endif()

# Static CRT on the PC (one exe, nothing to install); UWP packages must use the DLL CRT.
if(RF_UWP)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(rf_strict target)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    # NOGDI: wingdi.h declares a function named Rectangle, which collides with the raylib-shaped struct.
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN NOGDI)
endfunction()

# ---- shaders: compiled by fxc at build time into byte-array headers ---------
file(GLOB RF_SDK_BIN_DIRS "C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64")
list(SORT RF_SDK_BIN_DIRS)
list(REVERSE RF_SDK_BIN_DIRS)
find_program(RF_FXC fxc.exe PATHS ${RF_SDK_BIN_DIRS} REQUIRED)
set(RF_SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")
set(RF_SHADER_SRC "${CMAKE_SOURCE_DIR}/engine/src/d3d11/Sprite.hlsl")

function(rf_shader out entry profile var)
    add_custom_command(
        OUTPUT  "${RF_SHADER_DIR}/${out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RF_SHADER_DIR}"
        COMMAND "${RF_FXC}" /nologo /T ${profile} /E ${entry} /O3 /Vn ${var} /Fh "${RF_SHADER_DIR}/${out}" "${RF_SHADER_SRC}"
        DEPENDS "${RF_SHADER_SRC}"
        COMMENT "fxc ${entry} -> ${out}"
        VERBATIM)
endfunction()
rf_shader(Sprite_vs.h VSMain vs_4_0 kSpriteVS)
rf_shader(Sprite_ps.h PSMain ps_4_0 kSpritePS)

# ---- engine -----------------------------------------------------------------
set(RF_ENGINE_COMMON
    engine/src/Log.cpp
    engine/src/Random.cpp
    engine/src/input/Input_Common.cpp
    engine/src/storage/Storage_Common.cpp
    engine/src/text/Text_Pixel.cpp
    engine/src/d3d11/Graphics_D3D11.cpp
    engine/src/xaudio2/Audio_XAudio2.cpp
    "${RF_SHADER_DIR}/Sprite_vs.h"
    "${RF_SHADER_DIR}/Sprite_ps.h"
)
if(RF_UWP)
    set(RF_ENGINE_PLATFORM
        engine/src/uwp/Platform_Uwp.cpp
        engine/src/uwp/Pads_Uwp.cpp
        engine/src/uwp/Folder_Uwp.cpp
    )
else()
    set(RF_ENGINE_PLATFORM
        engine/src/win32/Platform_Win32.cpp
        engine/src/win32/Pads_XInput.cpp
        engine/src/win32/Folder_Win32.cpp
    )
endif()

add_library(rfengine STATIC ${RF_ENGINE_COMMON} ${RF_ENGINE_PLATFORM})
target_include_directories(rfengine PUBLIC engine/include PRIVATE "${RF_SHADER_DIR}")
if(RF_UWP)
    target_link_libraries(rfengine PUBLIC d3d11 dxgi xaudio2 windowsapp)
    target_compile_options(rfengine PRIVATE /bigobj)   # C++/WinRT headers are large
else()
    target_link_libraries(rfengine PUBLIC d3d11 dxgi xaudio2 xinput user32 shell32 ole32)
endif()
rf_strict(rfengine)

# ---- raylib compatibility layer --------------------------------------------
add_library(rfcompat STATIC compat/raylib_compat.cpp)
target_include_directories(rfcompat PUBLIC compat)
target_link_libraries(rfcompat PUBLIC rfengine)
rf_strict(rfcompat)

# ---- executables ------------------------------------------------------------
# On the PC a Release build is a windowed app with a plain main(); Debug keeps
# a console for the log. Under UWP the entry point is wWinMain in the engine's
# Platform_Uwp.cpp, which calls the same main() on the CoreWindow thread.
function(rf_app target)
    rf_strict(${target})
    if(RF_UWP)
        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE ON)
    else()
        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
        target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
    endif()
endfunction()

add_executable(demo demo/main.cpp)
target_link_libraries(demo PRIVATE rfengine)
rf_app(demo)

set(RF_GAME_DIR "${CMAKE_SOURCE_DIR}/../River-Flyer/src" CACHE PATH "River Flyer's src folder")
if(EXISTS "${RF_GAME_DIR}/Game.cpp")
    file(GLOB RF_GAME_SOURCES CONFIGURE_DEPENDS "${RF_GAME_DIR}/*.cpp")
    if(RF_UWP)
        # Our own manifest and tile art instead of CMake's generic ones. The
        # images are package content, deployed under Assets\ as the manifest expects.
        file(GLOB RF_UWP_ASSETS "${CMAKE_SOURCE_DIR}/uwp/Assets/*.png")
        set_source_files_properties(${RF_UWP_ASSETS} PROPERTIES VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "Assets")
        list(APPEND RF_GAME_SOURCES "${CMAKE_SOURCE_DIR}/uwp/Package.appxmanifest" ${RF_UWP_ASSETS})
    endif()
    add_executable(riverflyer ${RF_GAME_SOURCES})
    target_include_directories(riverflyer PRIVATE "${RF_GAME_DIR}")
    target_link_libraries(riverflyer PRIVATE rfcompat)
    rf_app(riverflyer)
    message(STATUS "riverflyer target: building the game from ${RF_GAME_DIR}")
else()
    message(STATUS "riverflyer target skipped: ${RF_GAME_DIR}/Game.cpp not found (set RF_GAME_DIR)")
endif()
