project(Sandbox)

add_executable(sandbox
    src/main.cpp
    src/SandboxApp.cpp
)

# Include directories for headers
target_include_directories(sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sandbox PRIVATE core)

# Copy resources folder to build directory
add_custom_target(copy_resources_sandbox ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:sandbox>/resources
)

# Copy scenes folder to build directory
add_custom_target(copy_scenes_sandbox ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scenes
    $<TARGET_FILE_DIR:sandbox>/scenes
)

add_dependencies(sandbox copy_resources_sandbox copy_scenes_sandbox)
