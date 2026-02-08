# ===========================================
# package.ps1 — Скрипт упаковки AutoClicker Suite
# ===========================================
# Создаёт:
#   1. Портативный ZIP-архив (не требует установки)
#   2. NSIS-установщик (опционально, требует NSIS)
#
# Использование:
#   .\scripts\package.ps1                              # ZIP + NSIS
#   .\scripts\package.ps1 -ZipOnly                     # Только ZIP
#   .\scripts\package.ps1 -QtPath "C:\Qt\6.7.0\msvc2019_64"

param(
    [string]$QtPath = "",
    [string]$BuildType = "Release",
    [string]$BuildDir = "",
    [switch]$ZipOnly,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if (-not $BuildDir) {
    $BuildDir = "$ProjectRoot\build"
}

# ==========================================
# Утилиты вывода
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
Write-Host "  AutoClicker Suite — Package Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ==========================================
# Считываем версию из CMakeLists.txt
# ==========================================
$cmakeContent = Get-Content "$ProjectRoot\CMakeLists.txt" -Raw
if ($cmakeContent -match 'project\(AutoClickerSuite\s+VERSION\s+(\d+\.\d+\.\d+)') {
    $Version = $Matches[1]
} else {
    $Version = "1.0.0"
}
Write-Host "  Версия: $Version"

# ==========================================
# Шаг 1: Сборка (если не пропущена)
# ==========================================
if (-not $SkipBuild) {
    Write-Step "Запуск сборки Release..."
    $buildScript = "$ProjectRoot\scripts\build.ps1"
    $buildParams = @{
        BuildType = $BuildType
    }
    if ($QtPath) { $buildParams.QtPath = $QtPath }

    & $buildScript @buildParams
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Сборка не удалась."
        exit 1
    }
}

# ==========================================
# Определяем путь к .exe
# ==========================================
$exeDir = "$BuildDir\$BuildType"
$exePath = "$exeDir\AutoClickerSuite.exe"

if (-not (Test-Path $exePath)) {
    # Попробуем без подкаталога BuildType (для Ninja/MinGW)
    $exeDir = $BuildDir
    $exePath = "$BuildDir\AutoClickerSuite.exe"
}

if (-not (Test-Path $exePath)) {
    Write-Err "Исполняемый файл не найден: $exePath"
    Write-Err "Сначала выполните сборку: .\scripts\build.ps1 -BuildType Release"
    exit 1
}
Write-Ok "Исполняемый файл: $exePath"

# ==========================================
# Шаг 2: Создание каталога дистрибутива
# ==========================================
Write-Step "Подготовка каталога дистрибутива..."

$DistDir = "$ProjectRoot\dist"
$PackageDir = "$DistDir\AutoClickerSuite-$Version"

if (Test-Path $PackageDir) {
    Remove-Item -Recurse -Force $PackageDir
}
New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

# Копируем exe и все Qt DLL (windeployqt уже скопировал их рядом с exe)
Write-Step "Копирование файлов..."

# Копируем всё содержимое каталога сборки (exe + Qt DLL + плагины)
$filesToCopy = Get-ChildItem -Path $exeDir -Recurse
$totalFiles = 0
foreach ($item in $filesToCopy) {
    $relativePath = $item.FullName.Substring($exeDir.Length + 1)
    $destPath = Join-Path $PackageDir $relativePath

    if ($item.PSIsContainer) {
        New-Item -ItemType Directory -Force -Path $destPath | Out-Null
    } else {
        $destDir = Split-Path -Parent $destPath
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item $item.FullName -Destination $destPath
        $totalFiles++
    }
}
Write-Ok "Скопировано файлов: $totalFiles"

# Копируем переводы
$translationsDir = "$exeDir\translations"
if (Test-Path $translationsDir) {
    $destTransDir = "$PackageDir\translations"
    if (-not (Test-Path $destTransDir)) {
        New-Item -ItemType Directory -Force -Path $destTransDir | Out-Null
    }
    Copy-Item "$translationsDir\*.qm" -Destination $destTransDir -Force -ErrorAction SilentlyContinue
    Write-Ok "Переводы скопированы."
}

# Копируем документацию
Copy-Item "$ProjectRoot\README.md" -Destination $PackageDir -ErrorAction SilentlyContinue
Copy-Item "$ProjectRoot\LICENSE" -Destination $PackageDir -ErrorAction SilentlyContinue

# ==========================================
# Шаг 3: Удаляем ненужные файлы
# ==========================================
Write-Step "Очистка от лишних файлов..."

$junkPatterns = @("*.pdb", "*.ilk", "*.exp", "*.lib", "*.obj", "*.log")
foreach ($pattern in $junkPatterns) {
    Get-ChildItem -Path $PackageDir -Filter $pattern -Recurse | Remove-Item -Force
}
Write-Ok "Лишние файлы удалены."

# ==========================================
# Шаг 4: Создание портативного ZIP
# ==========================================
Write-Step "Создание ZIP-архива..."

$ZipPath = "$DistDir\AutoClickerSuite-$Version-portable-win64.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }

Compress-Archive -Path $PackageDir -DestinationPath $ZipPath -CompressionLevel Optimal
$zipSize = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Ok "ZIP: $ZipPath ($zipSize MB)"

# ==========================================
# Шаг 5: NSIS установщик (опционально)
# ==========================================
if (-not $ZipOnly) {
    $nsisExe = Get-Command makensis -ErrorAction SilentlyContinue
    if (-not $nsisExe) {
        # Пробуем стандартные пути
        $nsisPath = "${env:ProgramFiles(x86)}\NSIS\makensis.exe"
        if (-not (Test-Path $nsisPath)) {
            $nsisPath = "${env:ProgramFiles}\NSIS\makensis.exe"
        }
        if (Test-Path $nsisPath) {
            $nsisExe = $nsisPath
        }
    } else {
        $nsisExe = $nsisExe.Path
    }

    if ($nsisExe) {
        Write-Step "Создание NSIS-установщика..."

        $nsisScript = "$ProjectRoot\scripts\installer.nsi"
        if (Test-Path $nsisScript) {
            $nsisArgs = @(
                "/DVERSION=$Version",
                "/DSOURCE_DIR=$PackageDir",
                "/DOUTPUT_DIR=$DistDir",
                "/DPROJECT_ROOT=$ProjectRoot",
                $nsisScript
            )
            & $nsisExe @nsisArgs
            if ($LASTEXITCODE -eq 0) {
                $installerPath = "$DistDir\AutoClickerSuite-$Version-setup-win64.exe"
                if (Test-Path $installerPath) {
                    $installerSize = [math]::Round((Get-Item $installerPath).Length / 1MB, 2)
                    Write-Ok "Установщик: $installerPath ($installerSize MB)"
                }
            } else {
                Write-Warn "NSIS завершился с ошибкой. Установщик не создан."
            }
        } else {
            Write-Warn "Файл installer.nsi не найден. Пропускаем."
        }
    } else {
        Write-Warn "NSIS не найден. Установщик не будет создан."
        Write-Warn "Установите NSIS: https://nsis.sourceforge.io/Download"
    }
}

# ==========================================
# Итог
# ==========================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Упаковка завершена!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Каталог:  $DistDir" -ForegroundColor Green

if (Test-Path "$DistDir\*.zip") {
    Get-ChildItem "$DistDir\*.zip" | ForEach-Object {
        $s = [math]::Round($_.Length / 1MB, 2)
        Write-Host "  ZIP:      $($_.Name) ($s MB)" -ForegroundColor Green
    }
}
if (Test-Path "$DistDir\*setup*.exe") {
    Get-ChildItem "$DistDir\*setup*.exe" | ForEach-Object {
        $s = [math]::Round($_.Length / 1MB, 2)
        Write-Host "  Installer: $($_.Name) ($s MB)" -ForegroundColor Green
    }
}
Write-Host ""
