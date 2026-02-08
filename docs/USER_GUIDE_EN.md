# User Guide — AutoClicker Suite

## Table of Contents

1. [Installation & Launch](#installation--launch)
2. [Application Interface](#application-interface)
3. [Mouse Module](#mouse-module)
4. [Keyboard Module](#keyboard-module)
5. [Macro Engine](#macro-engine)
6. [Smart Modes](#smart-modes)
7. [Profiles & Settings](#profiles--settings)
8. [Hotkeys](#hotkeys)
9. [Troubleshooting](#troubleshooting)

---

## Installation & Launch

### Portable Version (Recommended)

1. Download `AutoClickerSuite-X.X.X-portable-win64.zip`
2. Extract to any folder
3. Run `AutoClickerSuite.exe`

All settings are stored next to the executable — perfect for USB drives.

### Installer

1. Download `AutoClickerSuite-X.X.X-setup-win64.exe`
2. Run and follow the installation wizard
3. The program will create shortcuts on the Desktop and Start Menu

### Running as Administrator

Some applications (games, fullscreen programs) may require administrator privileges:
- Right-click `AutoClickerSuite.exe` → **Run as administrator**
- Or in shortcut properties → **Compatibility** → **Run this program as an administrator**

---

## Application Interface

The main window is divided into:

- **Top panel**: Menu bar (File, View, Help)
- **Central area**: Module tabs (Mouse, Keyboard, Macros, Smart Modes)
- **Bottom panel**: Real-time action log

When the window is closed, the application **minimizes to the system tray** (notification area). To fully exit, use **right-click on tray icon → Exit** or **File → Exit**.

---

## Mouse Module

### Basic Settings

| Parameter | Description |
|-----------|-------------|
| **Mouse button** | Left (LMB), Right (RMB), or Middle (MMB) |
| **Click type** | Single, Double, or Hold |
| **Interval** | Time between clicks (1 ms to 1 hour) |

### Positioning Modes

- **Follow cursor** — clicks at the current mouse position
- **Fixed coordinates** — clicks at specified X, Y. Press "Capture" — you'll have 5 seconds to move the cursor to the desired point
- **Multi-point route** — sequence of coordinates. Add points via "Add point" or capture them one by one

### Limits

- **By click count** — stop after N clicks (0 = infinite)
- **By time** — stop after N seconds

### Randomization (Anti-detect)

- **Interval randomization** — random spread ±% from the set interval
- **Coordinate randomization** — random pixel offset from position

---

## Keyboard Module

### Operating Modes

#### 1. Single Key
Automatic pressing of a selected key at the set interval. Press "Capture Key" and press the desired key.

#### 2. Key Combination
Automatic pressing of a combination (e.g., Ctrl+C). Select modifiers (Ctrl, Shift, Alt, Win) and the main key.

#### 3. Hold Key
Holds down a selected key until you press Stop. Useful for games or applications that require a key to be held. The key is pressed (keydown) when you start and released (keyup) when you stop.

#### 4. Type Text
Character-by-character text input with configurable delay between characters. Supports Unicode and Cyrillic.

#### 5. Keyboard Macro
Record and play back a sequence of key presses. Press "Start Recording" to record a macro.

### Parameters

| Parameter | Description |
|-----------|-------------|
| **Interval** | Delay between presses (1 ms — 1 hour) |
| **Count** | Press limit (0 = infinite) |
| **Randomization** | Random interval spread ±% |

---

## Macro Engine

### Recording a Macro

1. Press **F7** or the record button
2. Perform mouse and keyboard actions
3. Press **F7** again to stop recording
4. The recorded macro will appear in the editor

During recording, the following are captured:
- Mouse movements (with filtering of minor movements)
- Mouse clicks
- Key presses and releases
- Delays between actions

### Visual Editor

The editor displays the macro as a list of actions with color coding:
- **Blue** — mouse actions
- **Green** — keyboard actions
- **Orange** — delays
- **Purple** — loops and control flow

Available operations:
- **Drag & drop** — reorder actions
- **Add** — new actions via the parameters panel
- **Remove** — select an action and press "Remove"
- **Edit** — select an action and modify parameters

### Action Types

| Action | Description |
|--------|-------------|
| **Mouse click** | Click specified button at coordinates |
| **Mouse move** | Move cursor to a point |
| **Key press** | Press and release a key |
| **Key down** | Press a key (without releasing) |
| **Key up** | Release a held key |
| **Key combo** | Press combination (Ctrl+C etc.) |
| **Type text** | Character-by-character input |
| **Delay** | Pause (fixed or random) |
| **Loop start** | Begin repeat section |
| **Loop end** | End repeat section |
| **Sub-macro** | Insert another macro |
| **Comment** | Explanatory text (not executed) |

### Loops

Nested loops allow repeating groups of actions:
```
[Loop: 5 times]
    Mouse click (100, 200)
    Delay 500 ms
    [Loop: 3 times]
        Key press A
        Delay 100 ms
    [End loop]
[End loop]
```
Maximum nesting depth: 10 levels.

### Playback Speed

Adjustable from **0.1x** (10 times slower) to **10x** (10 times faster).

### Export / Import

Macros are stored as JSON files. Use the "Export" and "Import" buttons to share macros.

---

## Smart Modes

### Window Binding

All actions will only execute when the specified window is active.

1. Press "Select Window"
2. Click on the desired window
3. Enable binding

### Color Search

Wait for a pixel of the desired color to appear (or disappear) on screen.

1. Specify the color (can be captured from screen)
2. Set tolerance (0–255 for each RGB channel)
3. Choose action on detection (click, double-click, move, notification)
4. Search area: entire screen or bound window

### Image Search

Search for an image fragment on screen (template matching).

1. Load a template (screenshot fragment)
2. Set match threshold (0.0–1.0, recommended 0.85+)
3. Choose action on detection
4. Works without OpenCV — built-in NCC implementation with pyramid optimization

### Scheduler

Run macros on a schedule.

1. Add a task
2. Set the run time (date + time)
3. Select the macro to execute
4. Optional: repeat interval (in minutes)

### Profiles

Save and load all settings of all modules:

- **File → Save Profile** — save current settings
- **File → Load Profile** — restore settings

Profiles are stored as JSON files in `%APPDATA%\AutoClickerSuite\profiles\`.

---

## Hotkeys

| Key | Action | Works Globally |
|-----|--------|---------------|
| **F6** | Start / Stop current action | Yes |
| **F7** | Start / Stop macro recording | Yes |
| **F8** | Emergency stop **all** actions | Yes |

Hotkeys work even when the application is minimized to tray.

### Custom Hotkeys

You can customize hotkeys via **View → Settings → Hotkeys** tab:

1. Open Settings (Ctrl+,)
2. Go to the "Hotkeys" tab
3. Click "Change" next to the hotkey you want to modify
4. Press the desired key combination
5. Click "Apply" or "OK"

Supported modifiers: Ctrl, Shift, Alt. Press Escape to cancel capture.

---

## Settings

Access settings via **View → Settings** (Ctrl+,).

### Hotkeys Tab
Configure global hotkey bindings for Start/Stop, Record Macro, and Emergency Stop.

### General Tab

| Setting | Description |
|---------|-------------|
| **Minimize to tray** | Minimize to system tray on window close |
| **Start minimized** | Launch minimized to tray |
| **Confirm exit** | Show confirmation dialog on exit |
| **Theme** | Dark (Catppuccin Mocha) or Light (Catppuccin Latte) |
| **Language** | Russian or English (requires restart) |
| **Max log lines** | Maximum number of lines in the log panel |
| **Log to file** | Enable file logging |

### About Tab
Shows version information, credits, and links.

---

## Troubleshooting

### Clicks don't work in a specific application

- Run AutoClicker Suite **as administrator**
- Some applications (especially games with anti-cheat) may block SendInput

### Application won't start

- Make sure **Visual C++ Redistributable 2022** (x64) is installed
- Check that all Qt DLLs are next to the .exe
- Try running as administrator

### Hotkeys don't work

- Make sure another program isn't intercepting F6/F7/F8
- Restart the application as administrator
- Check that the application is running (system tray icon)
- Customize hotkeys in Settings if there are conflicts

### High CPU usage during image search

- Increase the search interval
- Reduce the search area (bind to a window)
- Use a smaller template

### Log fills up too quickly

The log is automatically limited to 5000 lines. Log file: `%APPDATA%\AutoClickerSuite\logs\autoclicker.log`.

---

## System Requirements

| Parameter | Minimum | Recommended |
|-----------|---------|-------------|
| **OS** | Windows 10 x64 | Windows 11 x64 |
| **RAM** | 50 MB | 100 MB |
| **CPU** | Any x64 | — |
| **Disk** | ~30 MB | ~50 MB |
| **Resolution** | 1024×768 | 1920×1080+ |
