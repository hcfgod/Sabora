Param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# Ensure git is available
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required to download dependencies. Please install git and retry."
}

$root = (Resolve-Path "$PSScriptRoot\..").Path
$vendor = Join-Path $root "Engine\Vendor"
New-Item -ItemType Directory -Force -Path $vendor | Out-Null

function Clone-Or-Update($url, $targetDir) {
    if (Test-Path (Join-Path $targetDir ".git")) {
        if ($Force) {
            Write-Host "Resetting $targetDir ..."
            git -C $targetDir fetch --depth 1 origin
            git -C $targetDir reset --hard FETCH_HEAD
        } else {
            Write-Host "Updating $targetDir ..."
            git -C $targetDir pull --ff-only
        }
    } else {
        Write-Host "Cloning $url -> $targetDir ..."
        git clone --recursive --depth 1 $url $targetDir
    }
}

Clone-Or-Update "https://github.com/gabime/spdlog.git"    (Join-Path $vendor "spdlog")
Clone-Or-Update "https://github.com/doctest/doctest.git"   (Join-Path $vendor "doctest")
Clone-Or-Update "https://github.com/g-truc/glm.git"        (Join-Path $vendor "glm")
Clone-Or-Update "https://github.com/nlohmann/json.git"     (Join-Path $vendor "json")

Write-Host "Dependencies are ready under $vendor"