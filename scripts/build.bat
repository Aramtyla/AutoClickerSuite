@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: ===========================================
:: build.bat — Скрипт сборки AutoClicker Suite
:: ===========================================
:: Использование:
::   build.bat              — Release-сборка (по умолчанию)
::   build.bat debug        — Debug-сборка
::   build.bat release      — Release-сборка
::   build.bat clean        — Очистка build/
::   build.bat portable     — Портативная Release-сборка
::   build.bat installer    — Сборка Release + NSIS-установщик
::
:: Требования:
::   - CMake 3.24+
::   - Qt6 (переменная CMAKE_PREFIX_PATH или Qt6_DIR)
::   - MSVC (Visual Studio 2022) или MinGW
::   - NSIS (для команды installer)

set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build"
set "BUILD_TYPE=Release"
set "PORTABLE=OFF"
set "MAKE_INSTALLER=0"
set "GENERATOR="

:: ==========================================
:: Парсинг аргументов
:: ==========================================
if "%~1"=="" goto :detect_generator
if /I "%~1"=="debug" (
    set "BUILD_TYPE=Debug"
    goto :detect_generator
)
if /I "%~1"=="release" (
    set "BUILD_TYPE=Release"
    goto :detect_generator
)
if /I "%~1"=="clean" (
    echo [*] Очистка каталога сборки...
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
    echo [+] Готово.
    exit /b 0
)
if /I "%~1"=="portable" (
    set "BUILD_TYPE=Release"
    set "PORTABLE=ON"
    goto :detect_generator
)
if /I "%~1"=="installer" (
    set "BUILD_TYPE=Release"
    set "MAKE_INSTALLER=1"
    goto :detect_generator
)

echo [!] Неизвестная команда: %~1
echo Использование: build.bat [debug^|release^|clean^|portable^|installer]
exit /b 1

:: ==========================================
:: Определение генератора CMake
:: ==========================================
:detect_generator

:: Проверяем доступность CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [!] CMake не найден в PATH. Установите CMake 3.24+
    exit /b 1
)

:: Пытаемся найти Visual Studio
where cl >nul 2>&1
if not errorlevel 1 (
    set "GENERATOR=-G "Ninja""
    echo [*] Используется: Ninja + MSVC
    goto :configure
)

:: Пытаемся найти vswhere для автоматического поиска VS
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
    if defined VS_PATH (
        echo [*] Найдена Visual Studio: !VS_PATH!
        set "GENERATOR=-G "Visual Studio 17 2022" -A x64"
        goto :configure
    )
)

:: Fallback: Ninja или MinGW Makefiles
where ninja >nul 2>&1
if not errorlevel 1 (
    set "GENERATOR=-G "Ninja""
    echo [*] Используется: Ninja
    goto :configure
)

where mingw32-make >nul 2>&1
if not errorlevel 1 (
    set "GENERATOR=-G "MinGW Makefiles""
    echo [*] Используется: MinGW Makefiles
    goto :configure
)

echo [!] Не найден компилятор (MSVC, Ninja, MinGW). 
echo     Запустите из Developer Command Prompt или настройте PATH.
exit /b 1

:: ==========================================
:: Конфигурация CMake
:: ==========================================
:configure

echo.
echo ==========================================
echo   AutoClicker Suite — Сборка
echo   Тип: %BUILD_TYPE%
echo   Портативный: %PORTABLE%
echo ==========================================
echo.

:: Создаём каталог сборки
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/3] Конфигурация CMake...
cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" ^
    %GENERATOR% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DPORTABLE_MODE=%PORTABLE%

if errorlevel 1 (
    echo [!] Ошибка конфигурации CMake.
    exit /b 1
)

:: ==========================================
:: Сборка
:: ==========================================
echo [2/3] Компиляция...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo [!] Ошибка компиляции.
    exit /b 1
)

echo [3/3] Сборка завершена успешно!

:: ==========================================
:: Установщик (если запрошен)
:: ==========================================
if "%MAKE_INSTALLER%"=="1" (
    echo.
    echo [*] Создание установщика через CPack...
    cd /d "%BUILD_DIR%"
    cpack -G NSIS -C %BUILD_TYPE%
    if errorlevel 1 (
        echo [!] Ошибка создания установщика. Убедитесь, что NSIS установлен.
        exit /b 1
    )
    echo [+] Установщик создан в %BUILD_DIR%
)

:: ==========================================
:: Итог
:: ==========================================
echo.
echo ==========================================
echo   Сборка завершена!
echo   Исполняемый файл: build\%BUILD_TYPE%\AutoClickerSuite.exe
echo ==========================================

if "%PORTABLE%"=="ON" (
    echo.
    echo   Для создания портативного архива запустите:
    echo     scripts\package.bat
)

exit /b 0
