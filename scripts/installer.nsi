; ===========================================
; installer.nsi — NSIS-установщик AutoClicker Suite
; ===========================================
; Запуск: makensis /DVERSION=1.0.0 /DSOURCE_DIR=... /DOUTPUT_DIR=... /DPROJECT_ROOT=... installer.nsi

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

; ==========================================
; Параметры (передаются через /D)
; ==========================================
!ifndef VERSION
    !define VERSION "1.0.0"
!endif

!ifndef SOURCE_DIR
    !define SOURCE_DIR "..\dist\AutoClickerSuite-${VERSION}"
!endif

!ifndef OUTPUT_DIR
    !define OUTPUT_DIR "..\dist"
!endif

!ifndef PROJECT_ROOT
    !define PROJECT_ROOT ".."
!endif

; ==========================================
; Основные параметры
; ==========================================
Name "AutoClicker Suite ${VERSION}"
OutFile "${OUTPUT_DIR}\AutoClickerSuite-${VERSION}-setup-win64.exe"
InstallDir "$PROGRAMFILES64\AutoClickerSuite"
InstallDirRegKey HKLM "Software\AutoClickerSuite" "InstallDir"

; Запрос прав администратора
RequestExecutionLevel admin

; Сжатие
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; Кодировка
Unicode true

; ==========================================
; Метаданные
; ==========================================
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "AutoClicker Suite"
VIAddVersionKey "CompanyName" "AutoClickerSuite Team"
VIAddVersionKey "FileDescription" "AutoClicker Suite Installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "MIT License"

; ==========================================
; Интерфейс MUI2
; ==========================================
!define MUI_ABORTWARNING
!define MUI_ICON "${PROJECT_ROOT}\resources\icons\app.ico"
!define MUI_UNICON "${PROJECT_ROOT}\resources\icons\app.ico"

; Баннеры (если есть)
; !define MUI_HEADERIMAGE
; !define MUI_HEADERIMAGE_BITMAP "header.bmp"
; !define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"

; ==========================================
; Страницы установки
; ==========================================
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PROJECT_ROOT}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Страницы удаления
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; ==========================================
; Языки
; ==========================================
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

; ==========================================
; Секция установки
; ==========================================
Section "AutoClicker Suite" SecMain
    SectionIn RO  ; Обязательная секция

    SetOutPath "$INSTDIR"

    ; Копируем все файлы из каталога сборки
    File /r "${SOURCE_DIR}\*.*"

    ; Записываем путь установки в реестр
    WriteRegStr HKLM "Software\AutoClickerSuite" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\AutoClickerSuite" "Version" "${VERSION}"

    ; Добавляем запись для «Установка и удаление программ»
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "DisplayName" "AutoClicker Suite ${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "DisplayIcon" "$\"$INSTDIR\AutoClickerSuite.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "Publisher" "AutoClickerSuite Team"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "URLInfoAbout" "https://github.com/Aramtyla/AutoClickerSuite"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "NoRepair" 1

    ; Вычисляем размер установки
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite" \
        "EstimatedSize" "$0"

    ; Создаём деинсталлятор
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Ярлыки в меню Пуск
    CreateDirectory "$SMPROGRAMS\AutoClicker Suite"
    CreateShortCut "$SMPROGRAMS\AutoClicker Suite\AutoClicker Suite.lnk" \
        "$INSTDIR\AutoClickerSuite.exe" "" "$INSTDIR\AutoClickerSuite.exe" 0
    CreateShortCut "$SMPROGRAMS\AutoClicker Suite\Удалить AutoClicker Suite.lnk" \
        "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

    ; Ярлык на рабочем столе
    CreateShortCut "$DESKTOP\AutoClicker Suite.lnk" \
        "$INSTDIR\AutoClickerSuite.exe" "" "$INSTDIR\AutoClickerSuite.exe" 0

SectionEnd

; ==========================================
; Секция удаления
; ==========================================
Section "Uninstall"

    ; Удаляем файлы
    RMDir /r "$INSTDIR"

    ; Удаляем ярлыки
    Delete "$DESKTOP\AutoClicker Suite.lnk"
    RMDir /r "$SMPROGRAMS\AutoClicker Suite"

    ; Удаляем записи реестра
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoClickerSuite"
    DeleteRegKey HKLM "Software\AutoClickerSuite"

SectionEnd

; ==========================================
; Описания секций
; ==========================================
LangString DESC_SecMain ${LANG_RUSSIAN} "Основные файлы AutoClicker Suite"
LangString DESC_SecMain ${LANG_ENGLISH} "AutoClicker Suite core files"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ==========================================
; Функция инициализации
; ==========================================
Function .onInit
    ; Проверяем, что система 64-битная
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "AutoClicker Suite требует 64-битную версию Windows."
        Abort
    ${EndIf}

    ; Проверяем, не запущено ли приложение
    FindWindow $0 "" "AutoClicker Suite"
    ${If} $0 != 0
        MessageBox MB_OK|MB_ICONEXCLAMATION \
            "AutoClicker Suite уже запущен. Закройте приложение перед установкой."
        Abort
    ${EndIf}
FunctionEnd

Function un.onInit
    ; Подтверждение удаления
    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Вы уверены, что хотите удалить AutoClicker Suite?" \
        IDYES +2
        Abort
FunctionEnd
