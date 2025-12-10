Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Function to check if a command is available
function Test-Command($command) {
    return [bool](Get-Command $command -ErrorAction SilentlyContinue)
}

# Function to install CMake using winget
function Install-CMake {
    Write-Host "CMake not found. Installing CMake using winget..." -ForegroundColor Yellow
    
    if (Test-Command winget) {
        Write-Host "Installing CMake..." -ForegroundColor Green
        winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -eq 0) {
            Write-Host "CMake installed successfully!" -ForegroundColor Green
            Write-Host "Please restart your terminal/PowerShell and run this script again." -ForegroundColor Yellow
            exit 0
        } else {
            throw "Failed to install CMake using winget. Please install CMake manually from https://cmake.org/"
        }
    } else {
        throw "winget not available. Please install CMake manually from https://cmake.org/"
    }
}

# Function to install Python using winget (required for shaderc build)
function Install-Python {
    Write-Host "Python not found. Installing Python using winget..." -ForegroundColor Yellow
    
    if (Test-Command winget) {
        Write-Host "Installing Python..." -ForegroundColor Green
        winget install --id Python.Python.3.11 -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Python installed successfully!" -ForegroundColor Green
            Write-Host "Please restart your terminal/PowerShell and run this script again." -ForegroundColor Yellow
            exit 0
        } else {
            throw "Failed to install Python using winget. Please install Python manually from https://python.org/"
        }
    } else {
        throw "winget not available. Please install Python manually from https://python.org/"
    }
}

# Check and install CMake if needed
if (-not (Test-Command cmake)) {
    Install-CMake
}

# Check and install Python if needed (required for shaderc's utils/git-sync-deps)
if (-not (Test-Command python) -and -not (Test-Command python3)) {
    Install-Python
}

Write-Host "Required tools are available!" -ForegroundColor Green

$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$shadercDir = Join-Path $root "Engine\Vendor\shaderc"
$buildDir = Join-Path $shadercDir "build"

# Check if shaderc directory exists and has CMakeLists.txt
if (-not (Test-Path (Join-Path $shadercDir "CMakeLists.txt"))) {
    Write-Host "shaderc not found. Cloning shaderc..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone shaderc if it doesn't exist
    if (-not (Test-Path $shadercDir)) {
        Push-Location $vendorDir
        try {
            # Clone shaderc with submodules (includes SPIRV-Tools, SPIRV-Headers, glslang)
            git clone https://github.com/google/shaderc.git shaderc
            
            # Sync dependencies using shaderc's script
            Push-Location $shadercDir
            try {
                Write-Host "Syncing shaderc dependencies..."
                python utils/git-sync-deps
                
                # Patch glslang CMakeLists.txt to fix export issue with SPIRV-Tools-opt
                Write-Host "Patching glslang CMakeLists.txt to fix export issue..."
                $glslangCMake = Join-Path $shadercDir "third_party\glslang\CMakeLists.txt"
                if (Test-Path $glslangCMake) {
                    $content = Get-Content $glslangCMake -Raw
                    # Replace the problematic install(EXPORT) block with a commented version
                    # This fixes the issue where SPIRV-Tools-opt isn't in the export set
                    # Match install(EXPORT "glslang-targets" ...) including nested parentheses
                    $pattern = '(?s)(install\s*\(\s*EXPORT\s+"glslang-targets"[^)]*(?:\([^)]*\)[^)]*)*\))'
                    $replacement = '# Patched to fix SPIRV-Tools-opt export issue - install(EXPORT) disabled`n# $1'
                    $content = $content -replace $pattern, $replacement
                    # If the above didn't match, try a simpler approach - just comment out the line
                    if ($content -notmatch '# Patched to fix SPIRV-Tools-opt') {
                        $content = $content -replace '(install\s*\(\s*EXPORT\s+"glslang-targets")', '# Patched: $1'
                    }
                    Set-Content $glslangCMake -Value $content -NoNewline
                    Write-Host "Patched glslang CMakeLists.txt"
                }
            }
            finally {
                Pop-Location
            }
        }
        finally {
            Pop-Location
        }
    }
    
    # Verify CMakeLists.txt exists now
    if (-not (Test-Path (Join-Path $shadercDir "CMakeLists.txt"))) {
        throw "shaderc CMakeLists.txt not found after cloning. shaderc directory: $shadercDir"
    }
}

# Create build directory
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Write-Host "Building shaderc with CMake..."

# Configure shaderc with CMake
# Use CMAKE_SKIP_INSTALL_RULES to avoid glslang export issues with SPIRV-Tools
Write-Host "Configuring shaderc..."
& cmake -S "$shadercDir" -B "$buildDir" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
    -DCMAKE_SKIP_INSTALL_RULES=ON `
    -DSHADERC_SKIP_TESTS=ON `
    -DSHADERC_SKIP_EXAMPLES=ON `
    -DSHADERC_SKIP_INSTALL=ON `
    -DSHADERC_SKIP_COPYRIGHT_CHECK=ON `
    -DSPIRV_SKIP_TESTS=ON `
    -DSPIRV_SKIP_EXECUTABLES=OFF `
    -DENABLE_GLSLANG_BINARIES=OFF `
    -DBUILD_SHARED_LIBS=OFF

if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
}

# Build shaderc in both configurations
Write-Host "Building shaderc (Release)..."
& cmake --build $buildDir --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    throw "shaderc Release build failed with exit code $LASTEXITCODE"
}

Write-Host "Building shaderc (Debug)..."
& cmake --build $buildDir --config Debug --parallel

if ($LASTEXITCODE -ne 0) {
    throw "shaderc Debug build failed with exit code $LASTEXITCODE"
}

Write-Host "shaderc built successfully!"

# Copy the built libraries and headers to a location where premake5 can find them
$libDir = Join-Path $root "Engine\Vendor\shaderc\lib"
$includeDir = Join-Path $root "Engine\Vendor\shaderc\include"

New-Item -ItemType Directory -Force -Path $libDir | Out-Null
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null

# Copy shaderc libraries
$shadercLibRelease = Join-Path $buildDir "libshaderc\Release\shaderc_combined.lib"
$shadercLibDebug = Join-Path $buildDir "libshaderc\Debug\shaderc_combined.lib"

# Fallback to shaderc.lib if combined doesn't exist
if (-not (Test-Path $shadercLibRelease)) {
    $shadercLibRelease = Join-Path $buildDir "libshaderc\Release\shaderc.lib"
}
if (-not (Test-Path $shadercLibDebug)) {
    $shadercLibDebug = Join-Path $buildDir "libshaderc\Debug\shaderc.lib"
}

if (Test-Path $shadercLibRelease) {
    Copy-Item $shadercLibRelease (Join-Path $libDir "shaderc-release.lib") -Force
    Write-Host "Copied shaderc library (Release) to $libDir"
} else {
    Write-Warning "shaderc Release library not found at expected location"
    Get-ChildItem -Recurse -Path $buildDir -Name "*.lib" | Where-Object { $_ -match "shaderc" } | ForEach-Object { Write-Host "Found lib: $_" }
}

if (Test-Path $shadercLibDebug) {
    Copy-Item $shadercLibDebug (Join-Path $libDir "shaderc-debug.lib") -Force
    Write-Host "Copied shaderc library (Debug) to $libDir"
}

# Copy SPIRV-Tools libraries
$spirvToolsLibRelease = Join-Path $buildDir "third_party\spirv-tools\source\Release\SPIRV-Tools.lib"
$spirvToolsLibDebug = Join-Path $buildDir "third_party\spirv-tools\source\Debug\SPIRV-Tools.lib"

if (Test-Path $spirvToolsLibRelease) {
    Copy-Item $spirvToolsLibRelease (Join-Path $libDir "SPIRV-Tools-release.lib") -Force
    Write-Host "Copied SPIRV-Tools library (Release) to $libDir"
}

if (Test-Path $spirvToolsLibDebug) {
    Copy-Item $spirvToolsLibDebug (Join-Path $libDir "SPIRV-Tools-debug.lib") -Force
    Write-Host "Copied SPIRV-Tools library (Debug) to $libDir"
}

# Copy SPIRV-Tools-opt libraries
$spirvToolsOptLibRelease = Join-Path $buildDir "third_party\spirv-tools\source\opt\Release\SPIRV-Tools-opt.lib"
$spirvToolsOptLibDebug = Join-Path $buildDir "third_party\spirv-tools\source\opt\Debug\SPIRV-Tools-opt.lib"

if (Test-Path $spirvToolsOptLibRelease) {
    Copy-Item $spirvToolsOptLibRelease (Join-Path $libDir "SPIRV-Tools-opt-release.lib") -Force
    Write-Host "Copied SPIRV-Tools-opt library (Release) to $libDir"
}

if (Test-Path $spirvToolsOptLibDebug) {
    Copy-Item $spirvToolsOptLibDebug (Join-Path $libDir "SPIRV-Tools-opt-debug.lib") -Force
    Write-Host "Copied SPIRV-Tools-opt library (Debug) to $libDir"
}

# Copy headers
$shadercIncludeDir = Join-Path $shadercDir "libshaderc\include"
if (Test-Path $shadercIncludeDir) {
    Copy-Item -Path "$shadercIncludeDir\*" -Destination $includeDir -Recurse -Force
    Write-Host "Copied shaderc headers to $includeDir"
}

# Copy SPIRV-Tools headers
$spirvToolsIncludeDir = Join-Path $shadercDir "third_party\spirv-tools\include"
if (Test-Path $spirvToolsIncludeDir) {
    Copy-Item -Path "$spirvToolsIncludeDir\*" -Destination $includeDir -Recurse -Force
    Write-Host "Copied SPIRV-Tools headers to $includeDir"
}

# Copy SPIRV-Headers
$spirvHeadersIncludeDir = Join-Path $shadercDir "third_party\spirv-headers\include"
if (Test-Path $spirvHeadersIncludeDir) {
    Copy-Item -Path "$spirvHeadersIncludeDir\*" -Destination $includeDir -Recurse -Force
    Write-Host "Copied SPIRV-Headers to $includeDir"
}

Write-Host "shaderc build complete!" -ForegroundColor Green

