cmake_minimum_required(VERSION 3.20)
project(plotdraw LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE STRING "Executable suffix for emscripten")
endif()

find_package(SDL3 REQUIRED)
if(DEFINED imgui_DIR)
    find_package(imgui REQUIRED CONFIG HINTS ${imgui_DIR})
else()
    find_package(imgui REQUIRED CONFIG)
endif()

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/grid.cpp
    src/polygon.cpp
    src/property.cpp
    src/toolbox.cpp
    src/vertex_label.cpp
    src/utils/clipboard.cpp
    src/tools/plot_tool.cpp
    src/tools/select_tool.cpp
    src/tools/grid_tool.cpp
    src/tools/save_tool.cpp
    src/tools/load_tool.cpp
)

add_executable(plotdraw ${SOURCES})

if(EMSCRIPTEN)
    set_target_properties(plotdraw PROPERTIES SUFFIX ".html")
endif()

target_include_directories(plotdraw PRIVATE src)

target_link_libraries(plotdraw PRIVATE
    imgui::imgui
    SDL3::SDL3
)

if(EMSCRIPTEN)
    target_link_options(plotdraw PRIVATE
        -sUSE_SDL=3
        -sASYNCIFY
        # ccall is not exported by default, and its absence shows up as a runtime
        # "Module.ccall is not a function" rather than a link error. Naming any
        # EXPORTED_FUNCTIONS means _main has to be listed too or the app never starts.
        -sEXPORTED_RUNTIME_METHODS=ccall
        -sEXPORTED_FUNCTIONS=_main,_plotdraw_on_clipboard_read
        -sALLOW_MEMORY_GROWTH=0
        -sINITIAL_MEMORY=134217728
        -sUSE_WEBGL2=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -lGL
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell.html
    )
endif()

install(TARGETS plotdraw DESTINATION bin)

if(EMSCRIPTEN)
    install(FILES
        $<TARGET_FILE_DIR:plotdraw>/plotdraw.js
        $<TARGET_FILE_DIR:plotdraw>/plotdraw.wasm
        DESTINATION bin
    )
endif()
