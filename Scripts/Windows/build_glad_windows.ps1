Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Function to check if a command is available
function Test-Command($command) {
    return [bool](Get-Command $command -ErrorAction SilentlyContinue)
}

# Function to install Python using winget
function Install-Python {
    Write-Host "Python not found. Installing Python using winget..." -ForegroundColor Yellow
    
    if (Test-Command winget) {
        Write-Host "Installing Python..." -ForegroundColor Green
        winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Python installed successfully!" -ForegroundColor Green
            Write-Host "Please restart your terminal/PowerShell and run this script again." -ForegroundColor Yellow
            exit 0
        } else {
            throw "Failed to install Python using winget. Please install Python manually from https://www.python.org/"
        }
    } else {
        throw "winget not available. Please install Python manually from https://www.python.org/"
    }
}

# Check and install Python if needed
if (-not (Test-Command python)) {
    Install-Python
}

Write-Host "Python is available!" -ForegroundColor Green

# Install GLAD dependencies (jinja2 is required for GLAD 2.0)
Write-Host "Installing GLAD Python dependencies..." -ForegroundColor Green
python -m pip install --upgrade pip --quiet
python -m pip install jinja2 --quiet

if ($LASTEXITCODE -ne 0) {
    throw "Failed to install GLAD Python dependencies"
}

Write-Host "GLAD Python dependencies installed successfully" -ForegroundColor Green

$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$gladDir = Join-Path $root "Engine\Vendor\glad"
$gladIncludeDir = Join-Path $gladDir "include"
$gladSourceDir = Join-Path $gladDir "Source"

# Check if GLAD directory exists
if (-not (Test-Path $gladDir) -or $Force) {
    Write-Host "GLAD not found. Cloning GLAD..." -ForegroundColor Yellow
    
    # Ensure Vendor directory exists
    $vendorDir = Join-Path $root "Engine\Vendor"
    New-Item -ItemType Directory -Force -Path $vendorDir | Out-Null
    
    # Clone GLAD if it doesn't exist or Force is specified
    if (-not (Test-Path $gladDir) -or $Force) {
        Push-Location $vendorDir
        try {
            if ($Force -and (Test-Path $gladDir)) {
                Remove-Item -Recurse -Force $gladDir
            }
            git clone --depth 1 https://github.com/Dav1dde/glad.git glad
            Write-Host "Cloned GLAD repository" -ForegroundColor Green
        }
        finally {
            Pop-Location
        }
    }
}

# Create output directories
New-Item -ItemType Directory -Force -Path $gladIncludeDir | Out-Null
New-Item -ItemType Directory -Force -Path $gladSourceDir | Out-Null

# Generate GLAD loader for OpenGL 4.6 Core profile
Write-Host "Generating GLAD loader for OpenGL 4.6 Core..." -ForegroundColor Green

$gladGenerator = Join-Path $gladDir "glad\__main__.py"
$outputDir = Join-Path $gladDir "generated"

# Run GLAD generator
# Parameters:
# - API: gl:core=4.6 (OpenGL Core profile version 4.6)
# - Generator: c (C/C++) - specified as subcommand
# - Extensions: empty (no extensions for now, can be added later)
# - Output directory: generated
Push-Location $gladDir
try {
    python -m glad --api="gl:core=4.6" --out-path=$outputDir c
    
    if ($LASTEXITCODE -ne 0) {
        throw "GLAD generator failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "GLAD loader generated successfully" -ForegroundColor Green
}
finally {
    Pop-Location
}

# Copy generated files to include and Source directories
$generatedInclude = Join-Path $outputDir "include"
$generatedSource = Join-Path $outputDir "src"

# Debug: List what was generated
Write-Host "Checking generated output structure..." -ForegroundColor Yellow
if (Test-Path $outputDir) {
    Write-Host "Generated directory contents:" -ForegroundColor Cyan
    Get-ChildItem -Path $outputDir -Recurse -File | Select-Object -First 10 FullName | ForEach-Object { Write-Host "  $($_.FullName)" }
}

if (Test-Path $generatedInclude) {
    # Copy headers - preserve directory structure
    Get-ChildItem -Path $generatedInclude -Recurse | ForEach-Object {
        $relativePath = $_.FullName.Substring($generatedInclude.Length + 1)
        $destPath = Join-Path $gladIncludeDir $relativePath
        $destDir = Split-Path -Parent $destPath
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        if ($_.PSIsContainer -eq $false) {
            Copy-Item -Path $_.FullName -Destination $destPath -Force
        }
    }
    Write-Host "Copied GLAD headers to $gladIncludeDir" -ForegroundColor Green
} else {
    throw "Generated include directory not found: $generatedInclude"
}

if (Test-Path $generatedSource) {
    # Copy source files - preserve directory structure
    Get-ChildItem -Path $generatedSource -Recurse | ForEach-Object {
        $relativePath = $_.FullName.Substring($generatedSource.Length + 1)
        $destPath = Join-Path $gladSourceDir $relativePath
        $destDir = Split-Path -Parent $destPath
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        if ($_.PSIsContainer -eq $false) {
            Copy-Item -Path $_.FullName -Destination $destPath -Force
        }
    }
    Write-Host "Copied GLAD source files to $gladSourceDir" -ForegroundColor Green
} else {
    throw "Generated source directory not found: $generatedSource"
}

# Verify essential files exist - GLAD generates gl.h and gl.c (not glad.h/glad.c)
$gladHeaderPaths = @(
    (Join-Path $gladIncludeDir "glad\gl.h"),
    (Join-Path $gladIncludeDir "glad\glad.h"),
    (Join-Path $gladIncludeDir "gl.h"),
    (Join-Path $gladIncludeDir "glad.h"),
    (Join-Path $outputDir "include\glad\gl.h"),
    (Join-Path $outputDir "include\glad\glad.h")
)

$khrHeaderPaths = @(
    (Join-Path $gladIncludeDir "KHR\khrplatform.h"),
    (Join-Path $gladIncludeDir "khrplatform.h"),
    (Join-Path $outputDir "include\KHR\khrplatform.h"),
    (Join-Path $outputDir "include\khrplatform.h")
)

$gladSourcePaths = @(
    (Join-Path $gladSourceDir "gl.c"),
    (Join-Path $gladSourceDir "glad.c"),
    (Join-Path $gladSourceDir "src\gl.c"),
    (Join-Path $gladSourceDir "src\glad.c"),
    (Join-Path $outputDir "src\gl.c"),
    (Join-Path $outputDir "src\glad.c")
)

$gladHeaderFound = $false
foreach ($path in $gladHeaderPaths) {
    if (Test-Path $path) {
        Write-Host "Found GLAD header at: $path" -ForegroundColor Green
        $gladHeaderFound = $true
        break
    }
}

$khrHeaderFound = $false
foreach ($path in $khrHeaderPaths) {
    if (Test-Path $path) {
        Write-Host "Found khrplatform.h at: $path" -ForegroundColor Green
        $khrHeaderFound = $true
        break
    }
}

$gladSourceFound = $false
foreach ($path in $gladSourcePaths) {
    if (Test-Path $path) {
        Write-Host "Found GLAD source at: $path" -ForegroundColor Green
        $gladSourceFound = $true
        break
    }
}

if (-not $gladHeaderFound) {
    Write-Host "Warning: GLAD header (gl.h or glad.h) not found in expected locations. Searching..." -ForegroundColor Yellow
    $found = Get-ChildItem -Path $gladIncludeDir -Include "gl.h","glad.h" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) {
        Write-Host "Found GLAD header at: $($found.FullName)" -ForegroundColor Green
        $gladHeaderFound = $true
    } else {
        throw "Required GLAD header not found: gl.h or glad.h (searched in $gladIncludeDir)"
    }
}

if (-not $khrHeaderFound) {
    Write-Host "Warning: khrplatform.h not found in expected locations. Searching..." -ForegroundColor Yellow
    $found = Get-ChildItem -Path $gladIncludeDir -Filter "khrplatform.h" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) {
        Write-Host "Found khrplatform.h at: $($found.FullName)" -ForegroundColor Green
        $khrHeaderFound = $true
    } else {
        throw "Required GLAD file not found: khrplatform.h (searched in $gladIncludeDir)"
    }
}

if (-not $gladSourceFound) {
    Write-Host "Warning: GLAD source (gl.c or glad.c) not found in expected locations. Searching..." -ForegroundColor Yellow
    $found = Get-ChildItem -Path $gladSourceDir -Include "gl.c","glad.c" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) {
        Write-Host "Found GLAD source at: $($found.FullName)" -ForegroundColor Green
        $gladSourceFound = $true
    } else {
        throw "Required GLAD source not found: gl.c or glad.c (searched in $gladSourceDir)"
    }
}

Write-Host "GLAD build complete!" -ForegroundColor Green
Write-Host "GLAD headers: $gladIncludeDir" -ForegroundColor Cyan
Write-Host "GLAD source: $gladSourceDir" -ForegroundColor Cyan
