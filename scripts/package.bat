@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: ===========================================
:: package.bat — Упаковка портативного релиза
:: ===========================================
:: Создаёт готовый к распространению ZIP-архив с:
::   - AutoClickerSuite.exe (со статическим CRT)
::   - Qt DLL (через windeployqt)
::   - Переводы (.qm)
::   - README, LICENSE, CHANGELOG
::
:: Использование:
::   package.bat              — Упаковка из build/Release
::   package.bat [путь]       — Упаковка из указанного build-каталога

set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build"
set "RELEASE_DIR=%BUILD_DIR%\Release"
set "PACKAGE_NAME=AutoClickerSuite"
set "VERSION=1.0.0"
set "OUTPUT_DIR=%PROJECT_DIR%\dist"
set "PACKAGE_DIR=%OUTPUT_DIR%\%PACKAGE_NAME%-%VERSION%-portable-win64"

:: Переопределение пути к сборке
if not "%~1"=="" set "RELEASE_DIR=%~1"

:: ==========================================
:: Проверки
:: ==========================================
echo.
echo ==========================================
echo   Упаковка портативного релиза
echo   Версия: %VERSION%
echo ==========================================
echo.

:: Проверяем наличие исполняемого файла
set "EXE_PATH="
if exist "%RELEASE_DIR%\%PACKAGE_NAME%.exe" (
    set "EXE_PATH=%RELEASE_DIR%\%PACKAGE_NAME%.exe"
) else if exist "%BUILD_DIR%\%PACKAGE_NAME%.exe" (
    set "EXE_PATH=%BUILD_DIR%\%PACKAGE_NAME%.exe"
    set "RELEASE_DIR=%BUILD_DIR%"
) else (
    echo [!] Исполняемый файл не найден.
    echo     Сначала выполните сборку: scripts\build.bat release
    echo     Искали в: %RELEASE_DIR%
    exit /b 1
)

echo [*] Найден: %EXE_PATH%

:: ==========================================
:: Подготовка каталога
:: ==========================================
echo [1/5] Подготовка каталога пакета...

if exist "%PACKAGE_DIR%" rmdir /S /Q "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%"

:: ==========================================
:: Копирование файлов
:: ==========================================
echo [2/5] Копирование исполняемого файла и DLL...

:: Копируем exe
copy /Y "%EXE_PATH%" "%PACKAGE_DIR%\" >nul

:: Копируем все DLL из каталога сборки (Qt, плагины)
if exist "%RELEASE_DIR%\*.dll" (
    copy /Y "%RELEASE_DIR%\*.dll" "%PACKAGE_DIR%\" >nul 2>nul
)

:: Копируем плагины Qt (platforms, styles, imageformats и т.д.)
for %%D in (platforms styles imageformats iconengines tls networkinformation) do (
    if exist "%RELEASE_DIR%\%%D" (
        xcopy /E /I /Q /Y "%RELEASE_DIR%\%%D" "%PACKAGE_DIR%\%%D\" >nul 2>nul
    )
)

:: ==========================================
:: Копирование переводов
:: ==========================================
echo [3/5] Копирование переводов...

mkdir "%PACKAGE_DIR%\translations" 2>nul
if exist "%RELEASE_DIR%\translations\*.qm" (
    copy /Y "%RELEASE_DIR%\translations\*.qm" "%PACKAGE_DIR%\translations\" >nul
)

:: ==========================================
:: Копирование документации
:: ==========================================
echo [4/5] Копирование документации...

if exist "%PROJECT_DIR%\README.md"    copy /Y "%PROJECT_DIR%\README.md"    "%PACKAGE_DIR%\" >nul
if exist "%PROJECT_DIR%\LICENSE"      copy /Y "%PROJECT_DIR%\LICENSE"      "%PACKAGE_DIR%\" >nul
if exist "%PROJECT_DIR%\CHANGELOG.md" copy /Y "%PROJECT_DIR%\CHANGELOG.md" "%PACKAGE_DIR%\" >nul

:: ==========================================
:: Создание ZIP-архива
:: ==========================================
echo [5/5] Создание ZIP-архива...

set "ZIP_NAME=%PACKAGE_NAME%-%VERSION%-portable-win64.zip"
set "ZIP_PATH=%OUTPUT_DIR%\%ZIP_NAME%"

:: Удаляем старый архив
if exist "%ZIP_PATH%" del /Q "%ZIP_PATH%"

:: Используем PowerShell для создания ZIP
powershell -NoProfile -Command ^
    "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"

if errorlevel 1 (
    echo [!] Ошибка создания ZIP-архива.
    echo     Архив можно создать вручную из: %PACKAGE_DIR%
    exit /b 1
)

:: ==========================================
:: Подсчёт размера
:: ==========================================
for %%F in ("%ZIP_PATH%") do set "ZIP_SIZE=%%~zF"
set /A "ZIP_SIZE_MB=%ZIP_SIZE% / 1048576"

:: ==========================================
:: Итог
:: ==========================================
echo.
echo ==========================================
echo   Портативный релиз готов!
echo.
echo   Архив: dist\%ZIP_NAME%
echo   Размер: ~%ZIP_SIZE_MB% МБ
echo   Каталог: %PACKAGE_DIR%
echo.
echo   Содержимое:
echo     - AutoClickerSuite.exe
echo     - Qt6 DLL + плагины
echo     - Переводы (RU, EN)
echo     - README.md, LICENSE, CHANGELOG.md
echo ==========================================

exit /b 0
