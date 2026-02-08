# ===========================================
# build.ps1 — Скрипт сборки AutoClicker Suite
# ===========================================
# Использование:
#   .\scripts\build.ps1                           # Сборка Debug
#   .\scripts\build.ps1 -BuildType Release        # Сборка Release
#   .\scripts\build.ps1 -Portable                 # Портативная сборка
#   .\scripts\build.ps1 -Clean                    # Очистка + сборка
#   .\scripts\build.ps1 -QtPath "C:\Qt\6.7.0\msvc2019_64"

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release",

    [string]$QtPath = "",

    [switch]$Portable,
    [switch]$Clean,
    [switch]$NoBuild,
    [string]$Generator = "",
    [int]$Parallel = 0
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($Portable) { "$ProjectRoot\build-portable" } else { "$ProjectRoot\build" }

# ==========================================
# Определяем цвета для вывода
# ==========================================
function Write-Step($msg) { Write-Host "`n[*] $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "[+] $msg" -ForegroundColor Green }
function Write-Err($msg)  { Write-Host "[-] $msg" -ForegroundColor Red }
function Write-Warn($msg) { Write-Host "[!] $msg" -ForegroundColor Yellow }

# ==========================================
# Заголовок
# ==========================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AutoClicker Suite — Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Тип сборки:    $BuildType"
Write-Host "  Портативная:   $Portable"
Write-Host "  Каталог сборки: $BuildDir"
Write-Host ""

# ==========================================
# Проверяем наличие CMake
# ==========================================
Write-Step "Проверка зависимостей..."

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Err "CMake не найден. Установите CMake 3.24+ и добавьте в PATH."
    exit 1
}
$cmakeVersion = (cmake --version | Select-Object -First 1)
Write-Ok "CMake: $cmakeVersion"

# ==========================================
# Автоопределение Qt
# ==========================================
if (-not $QtPath) {
    # Попробуем найти Qt автоматически
    $possiblePaths = @(
        "C:\Qt\6.9.0\msvc2022_64",
        "C:\Qt\6.8.2\msvc2022_64",
        "C:\Qt\6.8.1\msvc2022_64",
        "C:\Qt\6.8.0\msvc2022_64",
        "C:\Qt\6.7.2\msvc2019_64",
        "C:\Qt\6.7.1\msvc2019_64",
        "C:\Qt\6.7.0\msvc2019_64",
        "C:\Qt\6.6.3\msvc2019_64",
        "C:\Qt\6.6.2\msvc2019_64",
        "C:\Qt\6.6.0\msvc2019_64",
        "C:\Qt\6.5.3\msvc2019_64",
        "C:\Qt\6.5.0\msvc2019_64"
    )

    foreach ($p in $possiblePaths) {
        if (Test-Path "$p\lib\cmake\Qt6") {
            $QtPath = $p
            break
        }
    }

    # Проверяем переменную окружения
    if (-not $QtPath -and $env:Qt6_DIR) {
        $QtPath = $env:Qt6_DIR
    }
    if (-not $QtPath -and $env:CMAKE_PREFIX_PATH) {
        $QtPath = $env:CMAKE_PREFIX_PATH
    }
}

if ($QtPath -and (Test-Path $QtPath)) {
    Write-Ok "Qt6 найден: $QtPath"
} else {
    Write-Warn "Qt6 не найден автоматически. CMake попробует найти его через PATH."
    Write-Warn "Укажите путь вручную: -QtPath 'C:\Qt\6.x.x\msvc20xx_64'"
}

# ==========================================
# Автоопределение генератора
# ==========================================
if (-not $Generator) {
    # Ищем Visual Studio
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsVersion = & $vsWhere -latest -property catalog_productLineVersion 2>$null
        if ($vsVersion -eq "2022") {
            $Generator = "Visual Studio 17 2022"
        } elseif ($vsVersion -eq "2019") {
            $Generator = "Visual Studio 16 2019"
        }
    }

    if (-not $Generator) {
        # Пробуем Ninja
        $ninja = Get-Command ninja -ErrorAction SilentlyContinue
        if ($ninja) {
            $Generator = "Ninja"
        } else {
            $Generator = "Visual Studio 17 2022"
        }
    }
}
Write-Ok "Генератор: $Generator"

# ==========================================
# Определяем кол-во потоков
# ==========================================
if ($Parallel -eq 0) {
    $Parallel = [Environment]::ProcessorCount
}
Write-Ok "Потоки сборки: $Parallel"

# ==========================================
# Очистка (если указан -Clean)
# ==========================================
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Step "Очистка каталога сборки..."
    Remove-Item -Recurse -Force $BuildDir
    Write-Ok "Каталог очищен."
}

# ==========================================
# Конфигурация CMake
# ==========================================
Write-Step "Конфигурация CMake..."

$cmakeArgs = @(
    "-B", $BuildDir,
    "-S", $ProjectRoot,
    "-G", $Generator
)

# Для Visual Studio — указываем архитектуру
if ($Generator -match "Visual Studio") {
    $cmakeArgs += @("-A", "x64")
}

# Для однопоточных генераторов — указываем тип сборки
if ($Generator -eq "Ninja" -or $Generator -eq "Unix Makefiles") {
    $cmakeArgs += @("-DCMAKE_BUILD_TYPE=$BuildType")
}

# Путь к Qt
if ($QtPath) {
    $cmakeArgs += @("-DCMAKE_PREFIX_PATH=$QtPath")
}

# Портативный режим
if ($Portable) {
    $cmakeArgs += @("-DPORTABLE_MODE=ON")
}

Write-Host "cmake $($cmakeArgs -join ' ')" -ForegroundColor DarkGray
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Err "Конфигурация CMake завершилась с ошибкой."
    exit 1
}
Write-Ok "Конфигурация завершена."

# ==========================================
# Сборка
# ==========================================
if (-not $NoBuild) {
    Write-Step "Сборка проекта ($BuildType)..."

    $buildArgs = @(
        "--build", $BuildDir,
        "--config", $BuildType,
        "--parallel", $Parallel
    )

    Write-Host "cmake $($buildArgs -join ' ')" -ForegroundColor DarkGray
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Сборка завершилась с ошибкой."
        exit 1
    }
    Write-Ok "Сборка завершена успешно!"

    # Определяем путь к исполняемому файлу
    $exePath = if ($Generator -match "Visual Studio") {
        "$BuildDir\$BuildType\AutoClickerSuite.exe"
    } else {
        "$BuildDir\AutoClickerSuite.exe"
    }

    if (Test-Path $exePath) {
        $fileSize = [math]::Round((Get-Item $exePath).Length / 1MB, 2)
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  Сборка готова!" -ForegroundColor Green
        Write-Host "  Файл: $exePath" -ForegroundColor Green
        Write-Host "  Размер: $fileSize MB" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
    }
}

Write-Host ""
