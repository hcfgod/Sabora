# Build Scripts

The Scripts directory contains platform-specific scripts for setting up and building Sabora. These scripts handle dependency installation, library building, and project generation automatically.

## What These Scripts Do

The scripts automate the entire setup process. They'll:

- Check for required tools (Git, CMake, etc.) and install them if missing
- Download and build all vendor dependencies
- Generate project files using Premake
- Build the engine and test projects

You don't need to manually install dependencies or configure build settings. Just run the setup script for your platform and everything is handled automatically.

## Platform Scripts

Scripts are organized by platform in separate directories:

- **Windows/** - PowerShell scripts (.ps1)
- **Linux/** - Bash scripts (.sh)
- **macOS/** - Bash scripts (.sh)

Each platform has scripts for installing dependencies, building individual libraries, and building the entire project.

## Usage

The easiest way to get started is to use the setup scripts in the root directory:

- **Windows**: `Setup.bat`
- **Linux**: `Setup_linux.sh`
- **macOS**: `Setup_macos.sh`

These scripts call the appropriate scripts in the Scripts directory automatically.

If you need to rebuild dependencies or work with individual libraries, you can run scripts directly from the platform directories. For example, to rebuild SDL3 on Windows:

```powershell
.\Scripts\Windows\build_sdl3_windows.ps1
```

## Automatic Tool Installation

The scripts will automatically install missing tools:

**Windows:**
- Uses winget to install Git and CMake if needed
- Requires Windows 10/11 with App Installer

**Linux:**
- Uses your system package manager (apt, dnf, yum, pacman, or zypper)
- Requires sudo access for package installation

**macOS:**
- Uses Homebrew to install tools
- Installs Homebrew itself if needed
- Requires an internet connection

## Script Permissions

On Linux and macOS, shell scripts need execute permissions. The setup scripts automatically fix permissions, but if you're running scripts directly, you may need to make them executable:

```bash
chmod +x Scripts/Linux/*.sh
chmod +x Scripts/macOS/*.sh
```

Or use the fix_script_permissions.sh script in each platform directory.

## Troubleshooting

If a script fails, check the error message. Common issues:

- **Missing permissions**: On Linux/macOS, you may need sudo for package installation
- **Network issues**: Downloading dependencies requires an internet connection
- **PATH issues**: After installing tools, you may need to restart your terminal

If you continue having problems, open an issue with details about your platform and the error message you're seeing.
