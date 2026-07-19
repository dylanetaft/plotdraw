cmake_minimum_required(VERSION 3.20)
project(plotdraw_superbuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

# Staging root that dependencies install into, and which the app then finds them in.
# Unrelated to ExternalProject's own PREFIX (build/<name>-prefix scratch dirs).
set(DEPS_DIR ${CMAKE_BINARY_DIR}/deps)

set(IMGUI_CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${DEPS_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

set(APP_CMAKE_ARGS
    -DCMAKE_PREFIX_PATH=${DEPS_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dist
    -Dimgui_DIR=${DEPS_DIR}/lib/cmake/imgui
)

if(EMSCRIPTEN)
    list(APPEND IMGUI_CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    )
    list(APPEND APP_CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_EXECUTABLE_SUFFIX=.html
    )
endif()

ExternalProject_Add(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW ON
    CMAKE_ARGS ${IMGUI_CMAKE_ARGS}
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/imgui_CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    BUILD_BYPRODUCTS
        ${DEPS_DIR}/lib/libimgui.a
    UPDATE_DISCONNECTED ON
)

# BUILD_ALWAYS: ExternalProject stamps the build step and does not watch app/src, so without
# this an edited source file is invisible to the outer build. The inner build is incremental,
# so an unchanged tree costs one no-op make.
ExternalProject_Add(app
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/app
    CMAKE_ARGS ${APP_CMAKE_ARGS}
    BUILD_ALWAYS ON
    DEPENDS imgui
)
