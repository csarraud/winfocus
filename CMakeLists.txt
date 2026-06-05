cmake_minimum_required(VERSION 3.16)
project(WinFocus VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GL REQUIRED gl)

# ImGui sources (bundled in ./imgui/)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(winfocus
    src/main.cpp
    src/x11_windows.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(winfocus PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${X11_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    src/
)

target_link_libraries(winfocus PRIVATE
    ${X11_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GL_LIBRARIES}
    dl
    pthread
)

target_compile_options(winfocus PRIVATE
    ${X11_CFLAGS_OTHER}
    ${GLFW_CFLAGS_OTHER}
)

# Install
install(TARGETS winfocus DESTINATION bin)
