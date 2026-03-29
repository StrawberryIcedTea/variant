cmake_minimum_required(VERSION 3.25)
project(variant LANGUAGES CXX C)

# ---- Standards ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Output: variant.dll ----
add_library(variant SHARED)

# ---- Sources ----
target_sources(variant PRIVATE
    src/main.cpp
    src/core/interfaces.cpp
    src/core/schema.cpp
    src/core/convars.cpp
    src/core/crc.cpp
    src/sdk/entity.cpp

    src/core/hooks/hooks.cpp
    src/core/hooks/render/render.cpp
    src/core/hooks/cursor/cursor.cpp
    src/core/hooks/createmove/createmove.cpp
    src/core/hooks/prediction/prediction.cpp
    src/core/hooks/viewmatrix/viewmatrix.cpp
    src/core/menu/menu.cpp
    src/features/movement/bhop/bhop.cpp
    src/features/movement/autostrafe/autostrafe.cpp
    src/features/movement/jumpbug/jumpbug.cpp
    src/features/movement/edgebug/edgebug.cpp
    src/features/movement/validate/validate.cpp
    src/utilities/debug.cpp
    src/utilities/memory.cpp
    src/utilities/input.cpp
)

# ImGui
target_sources(variant PRIVATE
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/imgui_impl_dx11.cpp
    dependencies/imgui/imgui_impl_win32.cpp
)

# MinHook (C sources)
target_sources(variant PRIVATE
    dependencies/minhook/buffer.c
    dependencies/minhook/hook.c
    dependencies/minhook/trampoline.c
    dependencies/minhook/hde/hde32.c
    dependencies/minhook/hde/hde64.c
)

# ---- Include dirs ----
target_include_directories(variant PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

# ---- Definitions ----
target_compile_definitions(variant PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ---- MSVC flags ----
if(MSVC)
    target_compile_options(variant PRIVATE
        /MP             # parallel compilation
        /permissive-    # strict conformance
        /Zi             # debug info
        /utf-8          # source encoding
        /EHsc           # C++ exceptions
        /W3             # warning level
        /wd4267         # conversion warnings
        /wd4244         # narrowing warnings
        /wd4828         # illegal character (minhook)
    )

    # Static runtime: /MT (Release), /MTd (Debug)
    set_property(TARGET variant PROPERTY MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Linker
    target_link_options(variant PRIVATE
        /DEBUG          # PDB output
        /MANIFEST:NO    # no manifest
    )
endif()

# ---- Link libraries ----
target_link_libraries(variant PRIVATE
    d3d11
    dxgi
    psapi      # GetModuleInformation
)
