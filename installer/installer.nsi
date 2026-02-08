; ===========================================
; installer.nsi — NSIS-установщик AutoClicker Suite
; ===========================================
; Для сборки: makensis installer.nsi
; Требуется: NSIS 3.x + плагины (включены по умолчанию)

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

; ==========================================
; Метаданные
; ==========================================
!define APP_NAME        "AutoClicker Suite"
!define APP_EXE         "AutoClickerSuite.exe"
!define APP_VERSION     "2.0.0"
!define APP_PUBLISHER   "AutoClickerSuite Team"
!define APP_URL         "https://github.com/Aramtyla/AutoClickerSuite"
!define APP_GUID        "{A1C2E3F4-5678-9ABC-DEF0-123456789ABC}"

; Каталог со скомпилированными файлами (передаётся через /D или по умолчанию)
!ifndef BUILD_DIR
    !define BUILD_DIR "..\build\Release"
!endif

; ==========================================
; Общие настройки
; ==========================================
Name "${APP_NAME} ${APP_VERSION}"
OutFile "..\dist\AutoClickerSuite-${APP_VERSION}-setup-win64.exe"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; Информация о версии в exe
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "Установщик ${APP_NAME}"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"
VIAddVersionKey "LegalCopyright"  "© 2026 ${APP_PUBLISHER}"

; ==========================================
; Иконки MUI
; ==========================================
!define MUI_ICON "..\resources\icons\app.ico"
!define MUI_UNICON "..\resources\icons\app.ico"

; ==========================================
; Страницы установщика (Modern UI 2)
; ==========================================
!define MUI_ABORTWARNING
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

; Установка
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Запустить ${APP_NAME}"
!insertmacro MUI_PAGE_FINISH

; Удаление
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; ==========================================
; Языки
; ==========================================
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

; ==========================================
; Секция: Установка
; ==========================================
Section "Основные файлы" SecMain
    SectionIn RO  ; Обязательная секция

    SetOutPath "$INSTDIR"

    ; Копируем все файлы из BUILD_DIR
    File /r "${BUILD_DIR}\*.*"

    ; Создаём деинсталлятор
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Записываем в реестр
    WriteRegStr HKLM "Software\${APP_NAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\${APP_NAME}" "Version" "${APP_VERSION}"

    ; Информация для "Установка и удаление программ"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "DisplayIcon" "$INSTDIR\${APP_EXE}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "URLInfoAbout" "${APP_URL}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "NoRepair" 1

    ; Размер установки
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" \
        "EstimatedSize" $0
SectionEnd

; ==========================================
; Секция: Ярлыки
; ==========================================
Section "Ярлыки" SecShortcuts
    ; Ярлык в меню Пуск
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\Удалить ${APP_NAME}.lnk" \
        "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

    ; Ярлык на рабочем столе
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
SectionEnd

; ==========================================
; Секция: Автозапуск (опционально)
; ==========================================
Section /o "Автозапуск с Windows" SecAutostart
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" \
        "${APP_NAME}" "$\"$INSTDIR\${APP_EXE}$\" --minimized"
SectionEnd

; ==========================================
; Описания секций
; ==========================================
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain}       "Основные файлы программы (обязательно)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts}   "Ярлыки в меню Пуск и на рабочем столе"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutostart}   "Автоматический запуск при входе в Windows"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ==========================================
; Секция: Удаление
; ==========================================
Section "Uninstall"
    ; Закрываем приложение если запущено
    nsExec::ExecToLog 'taskkill /F /IM ${APP_EXE}'

    ; Удаляем файлы
    RMDir /r "$INSTDIR"

    ; Удаляем ярлыки
    Delete "$DESKTOP\${APP_NAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APP_NAME}"

    ; Удаляем из автозагрузки
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"

    ; Удаляем записи реестра
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}"
    DeleteRegKey HKLM "Software\${APP_NAME}"
SectionEnd

; ==========================================
; Функции
; ==========================================
Function .onInit
    ; Проверяем, что система 64-битная
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "Эта программа требует 64-битную Windows."
        Abort
    ${EndIf}

    ; Проверяем, не запущено ли приложение
    FindWindow $0 "" "${APP_NAME}"
    ${If} $0 != 0
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "${APP_NAME} уже запущен. Закрыть и продолжить установку?" \
            IDYES close_app
        Abort
        close_app:
            nsExec::ExecToLog 'taskkill /F /IM ${APP_EXE}'
            Sleep 1000
    ${EndIf}
FunctionEnd
