# XFCE Launcher Plugin

A modern, full-screen application launcher plugin for XFCE Panel, inspired by macOS Launchpad. This plugin provides a visually appealing and efficient way to access all your installed applications with a single click.

## Version

0.7

## Overview

The XFCE Launcher Plugin transforms your desktop experience by providing a full-screen application grid that overlays your current workspace. It's designed to be fast, intuitive, and seamlessly integrated with the XFCE desktop environment.

## Key Features

### Visual Design
- **Full-screen overlay**: Covers your entire screen with a semi-transparent dark background
- **Adaptive grid layout**: The launcher adapts the grid, icon size, and spacing to your current screen resolution when opened
- **Smooth animations**: Hover effects and transitions provide visual feedback
- **Modern icons**: Displays application icons at optimal size with proper scaling
- **Clean interface**: Minimalist design that focuses on your applications

### Functionality
- **Instant search**: Start typing to filter applications in real-time
- **Search highlight**: Matching text inside app names is highlighted while searching
- **Smart filtering**: Search matches application names and descriptions
- **Quick launch**: Single click to launch any application
- **Keyboard navigation**:
  - Press ESC to close the launcher
  - Type to search without clicking the search box
  - Enter to launch the first search result
- **Hotkey support**: Assign a global shortcut to toggle the launcher
- **Drag & drop**:
  - Reorder applications by dragging
  - Drag one application onto another to create a folder (group)
  - Drag an application onto an existing folder to add it
- **Right-click context menu**: Access additional options for each application
- **Hide applications**: Right-click and select "Hide" to remove unwanted apps from view
- **Unhide functionality**: Hidden applications can be restored through settings
- **Snap support**: Automatically discovers and displays Snap applications
- **Flatpak support**: Automatically discovers and displays Flatpak applications
- **Live refresh**: Automatically updates when applications are installed or uninstalled

### Integration
- **Panel integration**: Adds seamlessly to your XFCE panel as a launcher button
- **Customizable panel icon**: Change the launcher button icon in the plugin settings
- **Theme aware**: Respects your GTK theme for consistent appearance
- **Icon theme support**: Uses your system icon theme for application icons
- **Desktop file compliance**: Reads standard .desktop files from system directories

### Performance
- **Lightweight**: Minimal resource usage when not active
- **Fast loading**: Applications are cached for instant display
- **Dynamic updates**: Automatically detects newly installed or removed applications
- **Efficient rendering**: Uses GTK3 for hardware-accelerated graphics

## Dependencies

You need the following development packages installed:

- gtk+-3.0
- libxfce4panel-2.0
- libxfce4util-1.0
- libxfconf-0
- gio-2.0
- gcc
- make
- pkg-config

On Arch Linux, install them with:
```bash
sudo pacman -S gtk3 xfce4-panel xfconf xfce4-dev-tools gcc make pkg-config
```

## Building

### Simple Build (Recommended)

Use the simple Makefile:

```bash
make
```

### Using Autotools

If you prefer to use the autotools build system:

```bash
mkdir m4
xdt-autogen
./configure --prefix=/usr
make
```

## Installation

### System-wide Installation (Recommended)

```bash
make
sudo make install
```

This installs the plugin to the system directories:
- Plugin library: `/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/` (on Debian/Ubuntu)
- Plugin library: `/usr/lib/xfce4/panel/plugins/` (on other distributions)
- Desktop file: `/usr/share/xfce4/panel/plugins/`

### Local User Installation

For testing without sudo privileges:

```bash
make
make install-local
```

This installs to your user directory (`~/.local/lib/xfce4/panel/plugins/`).

## Usage

1. After installation, restart XFCE Panel:
   ```bash
   xfce4-panel -r
   ```

2. Right-click on your XFCE panel and select "Panel" → "Add New Items..."

3. Find "Application Launcher" in the list and add it

4. Click the launcher icon in your panel to open the full-screen launcher

5. Type to search for applications or click on any app to launch it

6. Press ESC to close the launcher

Optional: Assign a hotkey in the plugin settings (right-click the launcher button → "Properties") to toggle the launcher.

## Uninstallation

For system-wide installation:
```bash
sudo make uninstall
```

For local user installation:
```bash
make uninstall-local
```

## Customization

### Hotkey
You can assign a global hotkey to toggle the launcher:
1. Right-click the launcher button in the panel
2. Select "Properties"
3. In the settings dialog, use the **Hotkey** section (Set/Clear)

### Styling
The plugin uses GTK CSS for styling. You can modify the appearance by editing the CSS in the source code (`src/ui.c`).

## Troubleshooting

If the plugin doesn't appear after installation:

1. Make sure XFCE Panel is restarted: `xfce4-panel -r`
2. Check if the plugin files are in the correct location:
   - System installation (Debian/Ubuntu):
     - Plugin library: `/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
   - System installation (other distributions):
     - Plugin library: `/usr/lib/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
   - Local installation:
     - Plugin library: `~/.local/lib/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `~/.local/share/xfce4/panel/plugins/xfce-launcher.desktop`
3. Run XFCE Panel in debug mode to see any errors: `xfce4-panel -q && PANEL_DEBUG=1 xfce4-panel`

## Changelog (Debian)

### 0.7 (2025-12-17)
- Added hotkey setting to toggle the launcher (XFCE custom shortcuts)
- Fixed and improved drag & drop (reorder, group into folders)
- Added search match highlighting
- Improved responsive layout for different screen resolutions
- Various stability and build warning fixes

### 0.6 (2025-07-23)
- Added customizable panel icon feature with settings dialog
- Implemented enhanced icon chooser with larger preview (48x48)
- Added file browser support for custom icons (SVG, PNG, JPEG)
- Added support for Snap and Flatpak applications
- Implemented automatic application refresh (no logout required)
- Added real-time monitoring of application directories
- Enhanced application discovery from multiple sources
- Added dependency on libxfconf for settings management
- Improved duplicate application detection
- Fixed icon display for both theme icons and file paths
- Enhanced settings dialog with better layout and spacing
- Added browse button for easy icon file selection
- Updated documentation with new features

### 0.5 (2025-07-23)
- Updated author information and copyright notices
- Added Debian packaging support
- Fixed minor issues and improved stability
- Added custom icon support with multiple sizes
- Updated documentation with comprehensive feature list

### 0.4 (2025-07-23)
- Initial release
- Full-screen application launcher plugin for XFCE Panel
- Features grid layout with search functionality
- Support for hiding applications
- Dynamic application detection

## Author

**Kamil 'Novik' Nowicki**
- Email: novik@axisos.org
- Website: www.axisos.org
- GitHub: https://github.com/Axis0S/xfce-panel-launcher

## License

Copyright (C) 2025 Kamil 'Novik' Nowicki

This plugin is provided as-is for educational and personal use.
