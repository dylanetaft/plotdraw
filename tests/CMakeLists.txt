cmake_minimum_required(VERSION 3.20)
project(plotdraw_tests LANGUAGES CXX)

# Deliberately NOT part of the superbuild. The app builds under the Emscripten toolchain; these
# are host binaries. Guarding them with if(NOT EMSCRIPTEN) inside app/CMakeLists.txt would mean
# they never build at all, since the normal build IS the emscripten one.
#
#   cmake -B build-tests tests
#   make -C build-tests -j32
#   ctest --test-dir build-tests --output-on-failure

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

set(APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../app/src)

# shim/ must precede app/src on the include path: it supplies a stub <imgui.h> so the app's
# non-rendering logic compiles against the host toolchain without SDL or a GL context.
function(add_plotdraw_test name)
    add_executable(${name} ${name}.cpp grid_stubs.cpp ${ARGN})
    target_include_directories(${name} BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/shim
        ${APP_SRC}
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_plotdraw_test(test_polygon
    ${APP_SRC}/polygon.cpp
)

add_plotdraw_test(test_observer
    ${APP_SRC}/polygon.cpp
)

add_plotdraw_test(test_labels
    ${APP_SRC}/vertex_label.cpp
)

add_plotdraw_test(test_actions
    ${APP_SRC}/polygon.cpp
    ${APP_SRC}/property.cpp
)

add_plotdraw_test(test_gestures
    ${APP_SRC}/polygon.cpp
    ${APP_SRC}/property.cpp
    ${APP_SRC}/tools/select_tool.cpp
    ${APP_SRC}/tools/plot_tool.cpp
)
