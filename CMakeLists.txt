cmake_minimum_required(VERSION 3.24)
project(AutoClickerSuite VERSION 1.0.0 LANGUAGES CXX)

# ==========================================
# Версия и метаданные для сборки
# ==========================================
set(APP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(APP_DESCRIPTION "Advanced AutoClicker Suite — автокликер мыши и клавиатуры с макросами")
set(APP_AUTHOR "AutoClickerSuite Team")
set(APP_URL "https://github.com/Aramtyla/AutoClickerSuite")
set(APP_LICENSE "MIT")

# Опция портативного режима (без реестра, настройки рядом с exe)
option(PORTABLE_MODE "Сборка портативной версии (настройки рядом с .exe)" OFF)

# ==========================================
# Настройки компиляции
# ==========================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)          # Автоматическая обработка Q_OBJECT
set(CMAKE_AUTORCC ON)          # Автоматическая обработка .qrc
set(CMAKE_AUTOUIC ON)          # Автоматическая обработка .ui

# Включаем оптимизации для Release
if(MSVC)
    add_compile_options(/W4 /utf-8)
    # Быстрая сборка
    add_compile_options(/MP)
    # Оптимизации для Release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
endif()

# LTO для GCC/Clang (на случай MinGW)
if(NOT MSVC)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "LTO включён для Release")
    endif()
endif()

# ==========================================
# Поиск Qt6
# ==========================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network        # Для автообновлений (в будущем)
    LinguistTools  # Для мультиязычности
)

# ==========================================
# Автогенерация version.h из шаблона
# ==========================================
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
    @ONLY
)

# ==========================================
# Исходные файлы
# ==========================================

# Утилиты
set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Settings.cpp
)
set(UTILS_HEADERS
    src/utils/Constants.h
    src/utils/Logger.h
    src/utils/Settings.h
)

# Ядро приложения
set(APP_SOURCES
    src/app/MainWindow.cpp
    src/app/TrayManager.cpp
    src/app/HotkeyManager.cpp
    src/app/ThemeManager.cpp
    src/app/LanguageManager.cpp
    src/app/ProfileManager.cpp
)
set(APP_HEADERS
    src/app/MainWindow.h
    src/app/TrayManager.h
    src/app/HotkeyManager.h
    src/app/ThemeManager.h
    src/app/LanguageManager.h
    src/app/ProfileManager.h
)

# Модуль мыши
set(MOUSE_SOURCES
    src/mouse/MouseWidget.cpp
    src/mouse/MouseClicker.cpp
)
set(MOUSE_HEADERS
    src/mouse/MouseWidget.h
    src/mouse/MouseClicker.h
    src/mouse/MouseConfig.h
)

# Модуль клавиатуры
set(KEYBOARD_SOURCES
    src/keyboard/KeyboardWidget.cpp
    src/keyboard/KeyboardClicker.cpp
)
set(KEYBOARD_HEADERS
    src/keyboard/KeyboardWidget.h
    src/keyboard/KeyboardClicker.h
    src/keyboard/KeyboardConfig.h
)

# Модуль макросов
set(MACRO_SOURCES
    src/macro/MacroWidget.cpp
    src/macro/MacroConfig.cpp
    src/macro/MacroRecorder.cpp
    src/macro/MacroPlayer.cpp
    src/macro/MacroEditor.cpp
)
set(MACRO_HEADERS
    src/macro/MacroWidget.h
    src/macro/MacroConfig.h
    src/macro/MacroRecorder.h
    src/macro/MacroPlayer.h
    src/macro/MacroEditor.h
)

# Умные режимы (полная реализация — Шаг 5)
set(SMART_SOURCES
    src/smart/SmartWidget.cpp
    src/smart/WindowFinder.cpp
    src/smart/ColorMatcher.cpp
    src/smart/ImageMatcher.cpp
    src/smart/Scheduler.cpp
)
set(SMART_HEADERS
    src/smart/SmartWidget.h
    src/smart/SmartConfig.h
    src/smart/WindowFinder.h
    src/smart/ColorMatcher.h
    src/smart/ImageMatcher.h
    src/smart/Scheduler.h
)

# Системный уровень
set(CORE_SOURCES
    src/core/InputSimulator.cpp
)
set(CORE_HEADERS
    src/core/InputSimulator.h
)

# ==========================================
# Ресурсы
# ==========================================
set(RESOURCE_FILES
    resources/resources.qrc
)

# Windows RC-файл (иконка, версия)
if(WIN32)
    set(WIN_RC resources/app.rc)
endif()

# Файлы переводов
set(TS_FILES
    resources/translations/app_en.ts
    resources/translations/app_ru.ts
)

# ==========================================
# Исполняемый файл
# ==========================================
qt_add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    ${UTILS_SOURCES}    ${UTILS_HEADERS}
    ${APP_SOURCES}      ${APP_HEADERS}
    ${MOUSE_SOURCES}    ${MOUSE_HEADERS}
    ${KEYBOARD_SOURCES} ${KEYBOARD_HEADERS}
    ${MACRO_SOURCES}    ${MACRO_HEADERS}
    ${SMART_SOURCES}    ${SMART_HEADERS}
    ${CORE_SOURCES}     ${CORE_HEADERS}
    ${RESOURCE_FILES}
    ${WIN_RC}
)

# ==========================================
# Мультиязычность — сборка .qm файлов из .ts
# ==========================================
qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# Копируем .qm файлы рядом с исполняемым файлом для доступа из ресурсов
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations"
    COMMENT "Создание каталога переводов..."
)

foreach(QM_FILE ${QM_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QM_FILE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/"
        COMMENT "Копирование файла перевода: ${QM_FILE}"
    )
endforeach()

# ==========================================
# Линковка
# ==========================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# Win32 API библиотеки
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
        shell32
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# ==========================================
# Информация о версии
# ==========================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
    $<$<BOOL:${PORTABLE_MODE}>:PORTABLE_MODE=1>
)

# ==========================================
# Установка (для windeployqt)
# ==========================================
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Установка — папка переводов
install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/translations"
    FILES_MATCHING PATTERN "*.qm"
)

# Автоматическое копирование Qt DLL (для Windows)
if(WIN32)
    # Находим windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-compiler-runtime
                --no-translations
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Запуск windeployqt для копирования Qt DLL..."
        )
    endif()
endif()

# ==========================================
# CPack — создание установщика и архива
# ==========================================
set(CPACK_PACKAGE_NAME "AutoClickerSuite")
set(CPACK_PACKAGE_VENDOR "${APP_AUTHOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${APP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${APP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${APP_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AutoClickerSuite")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Генераторы: ZIP для портативной, NSIS для установщика
set(CPACK_GENERATOR "ZIP;NSIS")

# Настройки NSIS
set(CPACK_NSIS_DISPLAY_NAME "AutoClicker Suite ${PROJECT_VERSION}")
set(CPACK_NSIS_PACKAGE_NAME "AutoClicker Suite")
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\AutoClickerSuite.exe")
set(CPACK_NSIS_URL_INFO_ABOUT "${APP_URL}")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH OFF)

# Создание ярлыка на рабочем столе (опционально)
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    CreateShortCut '$DESKTOP\\AutoClicker Suite.lnk' '$INSTDIR\\bin\\AutoClickerSuite.exe'
")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    Delete '$DESKTOP\\AutoClicker Suite.lnk'
")

# Ярлыки в меню Пуск
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\AutoClicker Suite.lnk' '$INSTDIR\\\\bin\\\\AutoClickerSuite.exe'"
)
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\AutoClicker Suite.lnk'"
)

include(CPack)

message(STATUS "==========================================")
message(STATUS "  AutoClicker Suite v${PROJECT_VERSION}")
message(STATUS "  Компилятор: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Стандарт: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Портативный: ${PORTABLE_MODE}")
message(STATUS "  Тип сборки: ${CMAKE_BUILD_TYPE}")
message(STATUS "==========================================")
