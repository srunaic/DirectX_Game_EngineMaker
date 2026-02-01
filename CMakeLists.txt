cmake_minimum_required(VERSION 3.20)
project(DirectXForgeEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Folder view in VS
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- Dependencies ---
include(FetchContent)

# ImGui (Docking Branch)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# --- Source Files ---
file(GLOB_RECURSE SOURCES 
    "Source/*.cpp" 
    "Source/*.h"
)
list(APPEND SOURCES
    "Source/Runtime/Scene/Scene.cpp"
    "Source/Runtime/Scene/Entity.cpp"
    "Source/Editor/EditorCamera.cpp"
    "Source/Editor/EditorCamera.h"
)

# --- Executable ---
# Added WIN32 to support wWinMain
add_executable(DirectXForgeEditor WIN32 ${SOURCES})

# Include Directories
target_include_directories(DirectXForgeEditor PUBLIC 
    Source
    Source/Runtime
    Source/Editor
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Add ImGui sources to the target (including backends)
target_sources(DirectXForgeEditor PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

# Link Libraries
target_link_libraries(DirectXForgeEditor PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Definitions
target_compile_definitions(DirectXForgeEditor PRIVATE UNICODE _UNICODE)

# Grouping files in IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${SOURCES})
