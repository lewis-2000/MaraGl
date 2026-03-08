# Timeline Panel & Bone Sequencer Guide

## Overview

The Timeline Panel provides a comprehensive system for viewing, analyzing, and managing skeletal animations in MaraGl. It features:

- **Automatic Bone Discovery**: Discovers all bones in loaded animations
- **Visual Sequencer**: Displays bone tracks with keyframe markers
- **Configurable Tracks**: Toggle bone visibility and organize tracks
- **Playback Controls**: Control animation playback with visual feedback
- **Time Navigation**: Scrub through animations with a timeline scrubber

## Accessing the Timeline Panel

1. Open your MaraGl application
2. In the ImGui editor interface, look for the **"Timeline"** panel
3. Select an entity with an AnimationComponent in the Hierarchy panel
4. The Timeline panel will update to show that entity's animation data

## Features

### 1. Timeline Controls (Top Section)

**Playback Controls**:

- **⏮ Start**: Jump to the beginning of the animation (0s)
- **▶/⏸ Play/Pause**: Toggle animation playback
- **⏭ End**: Jump to the end of the animation
- **Time Display**: Shows current time / total duration

**Timeline Scrubber**:

- Drag the slider to scrub through the timeline
- Adjust animation duration (0.5s - 30.0s range)

### 2. Skeletal Animations Section

Displays information about loaded animations:

- **Animation Selector**: Choose which animation to view
- **Animation Info**:
  - Total loaded animations
  - Active bones in current animation
  - Duration and playback speed
- **Animation Details (Collapsible)**:
  - Duration in seconds and ticks
  - Ticks per second (animation speed unit)
  - Number of bone channels
  - Complete bone list
  - Channel details (position/rotation/scale keys per bone)

**Sync with Timeline**:

- Checkbox to keep the timeline and skeletal animation in sync
- Useful for coordinating multiple animations across entities

### 3. Bone Sequencer (NEW)

The most powerful section for animation analysis and bone discovery.

#### Zoom Controls

```
Zoom slider: 10-500 pixels per second
Reset Zoom button: Returns to 100 px/sec default
```

#### Sequencer Display

The sequencer shows a visual timeline of all bones and their animations:

**Keyframe Indicators** (Color-Coded):

- 🔴 Red circles: Position keyframes
- 🟢 Green circles: Rotation keyframes
- 🔵 Blue circles: Scale keyframes

Each bone gets its own track showing when each type of animation occurs.

**Playback Head**:

- A yellow/gold vertical line shows current playback time
- Visible only when animation is playing or current time > 0

#### Track Configuration Panel

**Visibility Controls**:

- **Hide All / Show All**: Toggle visibility of all bone tracks at once
- Individual checkboxes for each bone
- Statistics: "Total Bones: X | Visible: Y"

**Bone Track Table**:

| Column        | Meaning                                                 |
| ------------- | ------------------------------------------------------- |
| **vis**       | Checkbox to toggle bone visibility in sequencer         |
| **Bone Name** | Name of the bone (clickable/selectable)                 |
| **Keys**      | Total number of keyframes (position + rotation + scale) |
| **Range**     | Time range of animations (min-max in seconds)           |

**Table Features**:

- Scrollable if many bones exist
- Click bone name to select/deselect
- Hover over row for tooltips
- Frozen header row for easy reference while scrolling

### 4. Property Animation Tracks Section

For custom entity transformations:

- Add keyframes for Position, Rotation, Scale
- Visualize custom animation tracks
- Interpolate between keyframes

## Workflow Examples

### Example 1: Analyzing a Character Animation

1. **Load Character Model**: Import a character with skeleton via ModelLoaderPanel
2. **Add AnimationComponent**: Right-click entity → "Add AnimationComponent"
3. **Load Animations**: In Inspector, click "Load Animations from Model"
4. **Open Timeline**: Select entity and view Timeline panel
5. **Explore Bones**:
   - Watch the Bone Sequencer populate with all bones
   - Zoom in/out to see animation details
   - Hide non-essential bones to focus on specific limbs

### Example 2: Comparing Multiple Animations

1. Select a character with multiple animations loaded
2. Use "Select Animation" dropdown to switch between animations
3. Watch the Bone Sequencer refresh with new bone data
4. Compare keyframe patterns between animations
5. Analyze timing differences visually

### Example 3: Timing Adjustments

1. Play animation at current speed
2. Use **Speed** slider to adjust playback speed (0.1x - 3.0x)
3. Scrub timeline to find specific keyframes
4. Note the time values in the Range column for timing references

## Technical Details

### Bone Discovery Process

When you select a different animation or entity with AnimationComponent:

1. `DiscoverBonesInAnimation()` is called
2. Iterates through all `BoneAnimation` channels in the Animation
3. Creates a `BoneSequencerTrack` for each unique bone name
4. Counts total keyframes (position + rotation + scale)
5. Calculates time range from all keyframe timestamps

### Data Structures

**BoneSequencerTrack**:

```cpp
struct BoneSequencerTrack {
    std::string boneName;      // Name of the bone
    bool visible;              // Track visibility toggle
    bool selected;             // Track selection state
    int keyframeCount;         // Total keyframes
    float minTime, maxTime;    // Time range in seconds
};
```

**BoneSequencerState**:

```cpp
struct BoneSequencerState {
    uint32_t entityID;                      // Associated entity
    int animationIndex;                     // Current animation
    std::vector<BoneSequencerTrack> tracks; // All bone tracks
    std::set<std::string> discoveredBones;  // Discovered bone names
    bool needsRefresh;                      // Refresh flag
};
```

### Rendering Approach

The sequencer uses ImGui's DrawList API directly for efficient rendering:

- Each track has a fixed height (24px)
- Keyframes are rendered as colored circles
- Playback head is a highlighted vertical line
- Timeline is calculated based on animation duration and zoom level

## Performance Notes

- **Efficient Discovery**: O(n) where n = number of bones in animation
- **Lazy Refresh**: Only recalculates when animation selection changes
- **Memory Light**: Stores only track metadata, not animation data
- **Scalable**: Handles 100+ bones smoothly with scrollable table

## Tips & Tricks

1. **Fast Zoom**: Use Zoom slider while paused to inspect keyframe clusters
2. **Track Focus**: Hide all bones except the one you're analyzing
3. **Timing Reference**: Check Range column to find animation segments
4. **Color Coding**: Red/green/blue dots help identify animation type changes
5. **Scrub & Observe**: Manually scrub while watching viewport to see live bone movements

## Troubleshooting

**"No animations loaded" message**:

- Ensure AnimationComponent exists on entity
- Click "Load Animations from Model" in Inspector

**No bones appear in sequencer**:

- Entity may not have a mesh or model with skeleton
- Check if model has skeletal rig in external tool

**Playback head not showing**:

- Click Play button to start animation
- Or scrub timeline to any non-zero time

**Too many bones cluttering view**:

- Use Hide All, then Show only bones you need
- Use table to toggle visibility by name
- Increase zoom for clearer spacing

## Future Enhancements

Potential additions to the Timeline Panel:

- Keyframe editing directly in sequencer
- Bone selection from 3D viewport
- Animation blending visualization
- Curve editor for ease-in/ease-out
- Export/import custom animation tracks
- Multi-track animation blending
