# Editor Application

This is the editor application for the MaraGl engine. It provides a visual editor interface for creating and managing 3D scenes.

## Project Structure

```
editor/
├── src/
│   ├── AppLayer.cpp/h         # Main application layer
│   ├── main.cpp               # Entry point
│   ├── panels/                # Editor UI panels
│   │   ├── EditorTimelinePanel.*    # Animation timeline
│   │   ├── HierarchyPanel.*         # Scene hierarchy tree
│   │   ├── InspectorPanel.*         # Component inspector
│   │   ├── ModelLoaderPanel.*       # Model loading interface
│   │   ├── SceneSettingsPanel.*     # Scene settings
│   │   ├── LoadingPanel.*           # Async loading indicator
│   │   ├── EditorPanel.h            # Base panel interface
│   │   ├── PanelManager.h           # Panel management
│   │   └── ScenePanel.h             # Scene viewport panel
│   ├── layers/                # Application layers
│   │   └── ImGuiLayer.*       # ImGui integration layer
│   └── scene/                 # Scene-related code
└── CMakeLists.txt             # Build configuration
```

## Building

The editor is built as part of the main CMake project:

```bash
cd build
cmake ..
cmake --build . -j 8
```

The executable will be output as `editor.exe` (or `editor` on Unix systems).

## Dependencies

- **core**: The MaraGl core engine library
- **ImGui**: For the editor UI
- **GLFW**: For windowing
- **Assimp**: For model loading
