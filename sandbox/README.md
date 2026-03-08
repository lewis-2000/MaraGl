# Sandbox Application

This is a lightweight runtime application for the MaraGl engine. It loads and displays scenes without the editor UI overhead.

## Key Differences from Editor

- **No ImGui**: Focused on scene rendering only
- **Direct rendering**: Renders directly to the screen (no framebuffer layer)
- **Performance**: Minimal overhead, ideal for gameplay and scene testing
- **Scene loading**: Loads scenes from `.json` files

## Features

- Scene loading and rendering
- Camera controls with keyboard and mouse
- Grid visualization for spatial reference
- Async asset loading
- Automatic scene loading on startup

## Project Structure

```
sandbox/
├── src/
│   ├── main.cpp           # Entry point
│   ├── SandboxApp.h/cpp   # Main application class
│   └── resources/         # Linked at runtime
└── CMakeLists.txt         # Build configuration
```

## Building

The sandbox builds as part of the main CMake project:

```bash
cd build
cmake ..
cmake --build . -j 8
```

The executable will be output as `sandbox.exe` (or `sandbox` on Unix systems).

## Running

```bash
./sandbox.exe
```

The sandbox will automatically load `scenes/example_scene.json` on startup. You can modify the default scene by editing the `LoadScene()` call in `SandboxApp::run()`.

## Camera Controls

- **WASD**: Move forward/left/backward/right
- **Space/Ctrl**: Move up/down
- **Mouse**: Look around

## Dependencies

- **core**: The MaraGl core engine library (rendering, scene, input)
- **GLFW**: For windowing and input
- **Assimp**: For model loading
- **OpenGL 3.3**: For graphics

## Use Cases

- Testing scene layouts and models
- Gameplay testing with minimal UI overhead
- Performance profiling
- Building standalone executables
