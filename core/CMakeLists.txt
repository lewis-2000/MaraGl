project(Core)

# ----------------------------
# Core library
# ----------------------------
add_library(core STATIC)

# Only source files (implementation)
target_sources(core PRIVATE
    src/Engine.cpp
    src/Mara/Window.cpp  
    src/Mara/AppLayer.cpp
    src/Mara/VBO.cpp
    src/Mara/VAO.cpp
    src/Mara/EBO.cpp
    src/Mara/Shader.cpp
    src/Mara/Texture.cpp
    src/Mara/Renderer.cpp
    src/Mara/ImGuiLayer.cpp
    src/Mara/Framebuffer.cpp
    src/Mara/Input.cpp
    src/Mara/PerspectiveCamera.cpp
 )

# ----------------------------
# Public headers
# ----------------------------
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Mara/include
)

# ----------------------------
# OpenGL
# ----------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(core PUBLIC OpenGL::GL)

# ----------------------------
# Vendor path
# ----------------------------
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

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
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