project(Core)

# ----------------------------
# Core library
# ----------------------------
add_library(core STATIC)

target_sources(core PRIVATE
    src/Engine.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ----------------------------
# OpenGL
# ----------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(core PUBLIC OpenGL::GL)

# =========================================================
# VENDOR PATH
# =========================================================
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

# ----------------------------
# GLFW
# ----------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(
    ${VENDOR_DIR}/glfw
    ${CMAKE_BINARY_DIR}/vendor/glfw
)

target_link_libraries(core PUBLIC glfw)

# ----------------------------
# GLAD (generated via CMake)
# ----------------------------
add_subdirectory(
    ${VENDOR_DIR}/glad/cmake
    ${CMAKE_BINARY_DIR}/vendor/glad
)

glad_add_library(glad STATIC
    API gl:core=4.6
)

target_link_libraries(core PUBLIC glad)

# ----------------------------
# GLM (header-only)
# ----------------------------
add_library(glm INTERFACE)

target_include_directories(glm INTERFACE
    ${VENDOR_DIR}/glm
)

target_link_libraries(core PUBLIC glm)

# ----------------------------
# ImGui (Docking)
# ----------------------------
add_library(imgui STATIC
    ${VENDOR_DIR}/imgui/imgui.cpp
    ${VENDOR_DIR}/imgui/imgui_draw.cpp
    ${VENDOR_DIR}/imgui/imgui_tables.cpp
    ${VENDOR_DIR}/imgui/imgui_widgets.cpp
    ${VENDOR_DIR}/imgui/imgui_demo.cpp

    ${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${VENDOR_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${VENDOR_DIR}/imgui
    ${VENDOR_DIR}/imgui/backends
)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

target_link_libraries(core PUBLIC imgui)

# ----------------------------
# Windows (optional but recommended)
# ----------------------------
if(WIN32)
    target_link_libraries(core PUBLIC opengl32)
endif()

# ----------------------------
# Linux system libs
# ----------------------------
if(UNIX AND NOT APPLE)
    target_link_libraries(core PUBLIC
        dl
        pthread
        X11
    )
endif()