project(Editor)

add_executable(editor
    src/main.cpp
    src/AppLayer.cpp
    src/layers/ImGuiLayer.cpp
    src/panels/EditorTimelinePanel.cpp
    src/panels/HierarchyPanel.cpp
    src/panels/InspectorPanel.cpp
    src/panels/ModelLoaderPanel.cpp
    src/panels/SceneSettingsPanel.cpp
    src/panels/LoadingPanel.cpp
)

# Include directories for headers
target_include_directories(editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/layers
)

target_link_libraries(editor PRIVATE core)

# Copy resources folder to build directory
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:editor>/resources
)

# Copy scenes folder to build directory
add_custom_target(copy_scenes ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scenes
    $<TARGET_FILE_DIR:editor>/scenes
)

add_dependencies(editor copy_resources copy_scenes)
