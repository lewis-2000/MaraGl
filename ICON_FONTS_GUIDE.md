# Icon Font Usage Guide

This editor now has built-in support for icon fonts. Currently configured for **FontAwesome 6 Free** icons.

## Setup

### 1. Download FontAwesome 6 Free

- Visit: https://fontawesome.com/download
- Download the "Free" version (it's free!)
- Extract the archive

### 2. Add Icon Font to Project

Copy either the **OTF** or **TTF** file from the downloaded FontAwesome package.

The app will look for these in order (whichever is found first):

**TTF Files:**

- `resources/fonts/FontAwesome6-Solid.ttf`
- `resources/fonts/FontAwesome/FontAwesome6-Solid.ttf`
- `resources/fonts/icons/FontAwesome6-Solid.ttf`

**OTF Files:** (OpenType - what you likely downloaded)

- `resources/fonts/FontAwesome6-Solid.otf`
- `resources/fonts/FontAwesome/FontAwesome6-Solid.otf`
- `resources/fonts/icons/FontAwesome6-Solid.otf`
- `resources/fonts/FontAwesome/solid.otf`

**Quick Setup:**
If you downloaded FontAwesome and have the OTF files, simply copy `FontAwesome6-Solid.otf` to:

```
resources/fonts/FontAwesome6-Solid.otf
```

Both TTF and OTF formats are fully supported.

### 3. Restart Application

The icon font will be automatically loaded on startup. You'll see a message in the console confirming successful loading.

## Usage in Code

### Method 1: Using Icon Constants (Recommended)

```cpp
#include "IconDefs.h"
#include "ImGuiLayer.h"

// Get the icon font
ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");

// Push the font before using icons
if (iconFont)
    ImGui::PushFont(iconFont);

// Use icon constants with labels
if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save")))
{
    // Handle save
}

// Use icons only
if (ImGui::Button(Icons::Play))
{
    // Handle play
}

// Pop font when done
if (iconFont)
    ImGui::PopFont();
```

### Method 2: Direct Icon Usage

```cpp
// Without the helper macro
ImGui::Button("\xEF\x83\x87 Save");  // FileSave icon
ImGui::Text("%s Play", "\xEF\x81\x8B");  // Play icon
```

## Available Icon Constants

The `IconDefs.h` header provides predefined constants for common icons:

### File Operations

- `Icons::FileNew` - New file
- `Icons::FileOpen` - Open file
- `Icons::FileSave` - Save file
- `Icons::FileImport` - Import
- `Icons::FileExport` - Export

### Edit Operations

- `Icons::Edit` - Edit
- `Icons::Undo` - Undo
- `Icons::Redo` - Redo
- `Icons::Copy` - Copy
- `Icons::Paste` - Paste
- `Icons::Cut` - Cut
- `Icons::Delete` - Delete

### View Operations

- `Icons::Eye` - Visible
- `Icons::EyeSlash` - Hidden
- `Icons::Expand` - Expand
- `Icons::Compress` - Shrink
- `Icons::Maximize` - Fullscreen

### Object Manipulation

- `Icons::Move` - Move
- `Icons::MoveUp` - Move up
- `Icons::MoveDown` - Move down
- `Icons::Rotate` - Rotate
- `Icons::Scale` - Scale
- `Icons::Lock` - Lock
- `Icons::Unlock` - Unlock

### Playback Controls

- `Icons::Play` - Play
- `Icons::Pause` - Pause
- `Icons::Stop` - Stop
- `Icons::StepBack` - Step back
- `Icons::StepForward` - Step forward

### UI Elements

- `Icons::Plus` - Plus
- `Icons::Minus` - Minus
- `Icons::Check` - Checkmark
- `Icons::Times` - X/Close
- `Icons::Search` - Search
- `Icons::Settings` - Settings/Gear
- `Icons::Refresh` - Refresh

### Lighting

- `Icons::Sun` - Day/Brightness
- `Icons::Moon` - Night/Dark
- `Icons::Lightbulb` - Light

### Navigation

- `Icons::ChevronRight` - Right arrow
- `Icons::ChevronLeft` - Left arrow
- `Icons::Home` - Home

### Information

- `Icons::Info` - Information (i)
- `Icons::Warning` - Warning (!)
- `Icons::Error` - Error (X)
- `Icons::Question` - Help (?)

### Hierarchy/Objects

- `Icons::Folder` - Closed folder
- `Icons::FolderOpen` - Open folder
- `Icons::Cube` - 3D object
- `Icons::Layer` - Layer group

### Animation

- `Icons::Film` - Film/Video
- `Icons::Clock` - Time

## Helper Functions

```cpp
// Combine icon with label text
std::string buttonText = Icons::Icon(Icons::FileSave, "Save");
ImGui::Button(buttonText.c_str());

// Icon only (no label)
std::string iconOnly = Icons::IconOnly(Icons::Play);
ImGui::Button(iconOnly.c_str());
```

## Font Management

You can also load additional icon fonts programmatically:

```cpp
// Load a custom icon font
ImFont* customIcons = imGuiLayer->LoadIconFont(
    "resources/fonts/CustomIcons.ttf",
    16.0f,
    "CustomIcons"
);

// Check if font was loaded
if (imGuiLayer->HasIconFont("CustomIcons"))
{
    ImGui::PushFont(imGuiLayer->GetIconFont("CustomIcons"));
    ImGui::Text("Custom icon stuff");
    ImGui::PopFont();
}
```

## Finding Icon Codes

To find the Unicode codepoint for any FontAwesome icon:

1. Visit: https://fontawesome.com/icons
2. Search for the icon you want
3. Copy the Unicode value (e.g., `f0c7` for save)
4. Convert to UTF-8 bytes (usually \xEF\x83\x87 format)
5. Add to `IconDefs.h`

Alternatively, use this Python snippet to convert FontAwesome unicode to UTF-8:

```python
def fa_to_utf8(codepoint):
    # codepoint is hex like "f0c7"
    cp = int(codepoint, 16)
    # Convert to UTF-8 bytes
    bytes_val = chr(cp).encode('utf-8')
    return ''.join(f'\\x{b:02x}' for b in bytes_val)

print(fa_to_utf8('f0c7'))  # Output: \xef\x83\x87
```

## Troubleshooting

- **Icons not showing**: Make sure the TTF file is in the correct location and restart the application
- **Font looks pixelated**: Try a larger font size in `LoadIconFont()` or adjust the `fontSize` parameter
- **Missing icons**: The icon constant might not be defined - check `IconDefs.h` or add it manually

## Using Icons in Different Contexts

### ImGui Buttons

```cpp
if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save")))
{
    // Save action
}
```

### Menu Items

```cpp
ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
ImGui::PushFont(iconFont);

if (ImGui::MenuItem(Icons::Icon(Icons::FileOpen, "Open")))
{
    // Open action
}

ImGui::PopFont();
```

### Tree Nodes

```cpp
ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
ImGui::PushFont(iconFont);

if (ImGui::TreeNode(Icons::Icon(Icons::Folder, "Assets")))
{
    ImGui::Text("Asset contents...");
    ImGui::TreePop();
}

ImGui::PopFont();
```

### Text with Icons

```cpp
ImGui::Text("%s Status: Active", Icons::Check);
ImGui::TextColored(ImVec4(1,0,0,1), "%s Error: Something went wrong", Icons::Error);
```

## Performance Notes

- Font switching is cheap, do it as needed
- Icon fonts are cached after first load
- Multiple icon fonts can be loaded simultaneously
- No performance penalty for unused icons
