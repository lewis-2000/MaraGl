project(Core)

# ----------------------------
# Vendor path
# ----------------------------
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

# ----------------------------
# Core library
# ----------------------------
add_library(core STATIC)

# Only source files (implementation)
target_sources(core PRIVATE
    src/Engine.cpp
    src/Mara/Window.cpp  
    src/Mara/VBO.cpp
    src/Mara/VAO.cpp
    src/Mara/EBO.cpp
    src/Mara/Shader.cpp
    src/Mara/Texture.cpp
    src/Mara/Renderer.cpp
    src/Mara/Framebuffer.cpp
    src/Mara/Input.cpp
    src/Mara/PerspectiveCamera.cpp
    src/Mara/Mesh.cpp
    src/Mara/Model.cpp
    src/Mara/Timer.cpp
    src/Mara/Entity.cpp
    src/Mara/Scene.cpp
    src/Mara/Plane.cpp
    src/Mara/Skybox.cpp
    src/Mara/LightGizmo.cpp
    src/Mara/SceneSerializer.cpp
    src/Mara/Animator.cpp
    src/Mara/AnimationAsset.cpp
    src/Mara/AssetLoader.cpp
    ${VENDOR_DIR}/imnodes/imnodes.cpp
 )

# ----------------------------
# Public headers
# ----------------------------
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Mara/include
    ${VENDOR_DIR}/nlohmann
    ${VENDOR_DIR}/tinyexr
    ${VENDOR_DIR}/imnodes
    ${VENDOR_DIR}/assimp/contrib/zip/src
)

# ----------------------------
# OpenGL
# ----------------------------
find_package(OpenGL)
if(UNIX AND NOT APPLE AND NOT OpenGL_FOUND)
    message(WARNING "OpenGL development files were not found. On Ubuntu install: sudo apt install libgl1-mesa-dev")
endif()
if(TARGET OpenGL::GL)
    target_link_libraries(core PUBLIC OpenGL::GL)
elseif(TARGET OpenGL::OpenGL)
    target_link_libraries(core PUBLIC OpenGL::OpenGL)
endif()

# ----------------------------
# GLFW
# ----------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(MARAGL_LINUX_BACKEND "AUTO" CACHE STRING "Linux window backend: AUTO, WAYLAND, or X11")
set_property(CACHE MARAGL_LINUX_BACKEND PROPERTY STRINGS AUTO WAYLAND X11)

if(UNIX AND NOT APPLE)
    string(TOUPPER "${MARAGL_LINUX_BACKEND}" MARAGL_LINUX_BACKEND_NORMALIZED)

    if(MARAGL_LINUX_BACKEND_NORMALIZED STREQUAL "WAYLAND")
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    elseif(MARAGL_LINUX_BACKEND_NORMALIZED STREQUAL "X11")
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    elseif(MARAGL_LINUX_BACKEND_NORMALIZED STREQUAL "AUTO")
        find_program(MARAGL_WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
        if(MARAGL_WAYLAND_SCANNER_EXECUTABLE)
            set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
            set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
        else()
            set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        endif()
    else()
        message(FATAL_ERROR "Invalid MARAGL_LINUX_BACKEND='${MARAGL_LINUX_BACKEND}'. Use AUTO, WAYLAND, or X11.")
    endif()

    if(GLFW_BUILD_WAYLAND)
        find_program(MARAGL_WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
        if(NOT MARAGL_WAYLAND_SCANNER_EXECUTABLE)
            message(FATAL_ERROR
                "Wayland backend selected, but 'wayland-scanner' was not found. "
                "On Ubuntu install: sudo apt install libwayland-dev wayland-protocols")
        endif()
        message(STATUS "MaraGl Linux backend: Wayland")
    else()
        message(STATUS "MaraGl Linux backend: X11")
    endif()
endif()

add_subdirectory(
    ${VENDOR_DIR}/glfw
    ${CMAKE_BINARY_DIR}/vendor/glfw
)

target_link_libraries(core PUBLIC glfw)

# ----------------------------
# GLAD (prebuilt)
# ----------------------------
# Use pre-generated glad.c and headers
add_library(glad STATIC
    ${VENDOR_DIR}/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${VENDOR_DIR}/glad/include
)

target_link_libraries(core PUBLIC glad)

# ----------------------------
# GLM
# ----------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${VENDOR_DIR}/glm
)
target_link_libraries(core PUBLIC glm)

# ----------------------------
# Assimp
# ----------------------------
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    ${VENDOR_DIR}/assimp
    ${CMAKE_BINARY_DIR}/vendor/assimp
)

target_link_libraries(core PUBLIC assimp)

# ----------------------------
# ImGui docking
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
     ${VENDOR_DIR}/stb
)

target_link_libraries(imgui PUBLIC glfw)
if(TARGET OpenGL::GL)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
elseif(TARGET OpenGL::OpenGL)
    target_link_libraries(imgui PUBLIC OpenGL::OpenGL)
endif()
target_link_libraries(core PUBLIC imgui)

# ----------------------------
# Platform-specific libs
# ----------------------------
if(WIN32)
    target_link_libraries(core PUBLIC opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(core PUBLIC
        dl
        pthread
        X11
    )
endif()