project(Sandbox)

add_executable(sandbox
    src/main.cpp
)

target_link_libraries(sandbox PRIVATE core)

# Copy resources folder to build directory
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:sandbox>/resources
)

add_dependencies(sandbox copy_resources)