#pragma once
// ===========================================
// Constants.h — Глобальные константы приложения
// ===========================================

#include <QString>
#include <QKeySequence>

namespace AppConstants {

// ==========================================
// Информация о приложении
// ==========================================
inline constexpr const char* APP_DISPLAY_NAME = "AutoClicker Suite";
inline constexpr const char* APP_ORGANIZATION = "AutoClickerSuite";
inline constexpr const char* APP_DOMAIN       = "autoclickersuite.local";

// ==========================================
// Ограничения и значения по умолчанию
// ==========================================
namespace Mouse {
    inline constexpr int MIN_INTERVAL_MS      = 1;       // Минимальный интервал клика (мс)
    inline constexpr int MAX_INTERVAL_MS      = 3600000; // Максимальный интервал (1 час)
    inline constexpr int DEFAULT_INTERVAL_MS  = 100;     // Интервал по умолчанию
    inline constexpr int DEFAULT_CLICK_COUNT  = 0;       // 0 = бесконечно
    inline constexpr int MAX_CLICK_COUNT      = 1000000;
    inline constexpr int RANDOM_OFFSET_MAX    = 50;      // Макс. случайное смещение координат (пикс.)
}

namespace Keyboard {
    inline constexpr int MIN_INTERVAL_MS      = 1;
    inline constexpr int MAX_INTERVAL_MS      = 3600000;
    inline constexpr int DEFAULT_INTERVAL_MS  = 50;
    inline constexpr int DEFAULT_TYPE_DELAY   = 30;      // Задержка между символами при вводе текста
    inline constexpr int MAX_TYPE_DELAY       = 5000;
}

namespace Macro {
    inline constexpr int MAX_ACTIONS          = 10000;   // Макс. действий в макросе
    inline constexpr int MAX_LOOP_COUNT       = 100000;  // Макс. повторений цикла
    inline constexpr int MAX_NESTED_DEPTH     = 10;      // Макс. глубина вложенности
}

// ==========================================
// Глобальные хоткеи (по умолчанию)
// ==========================================
namespace Hotkeys {
    // ID для RegisterHotKey Win32
    inline constexpr int ID_START_STOP        = 1001;
    inline constexpr int ID_RECORD_MACRO      = 1002;
    inline constexpr int ID_EMERGENCY_STOP    = 1003;
    inline constexpr int ID_EXIT_APP          = 1004;

    // Комбинации по умолчанию
    inline const QKeySequence DEFAULT_START_STOP    = QKeySequence(Qt::Key_F6);
    inline const QKeySequence DEFAULT_RECORD        = QKeySequence(Qt::Key_F7);
    inline const QKeySequence DEFAULT_EMERGENCY     = QKeySequence(Qt::Key_F8);
}

// ==========================================
// Пути к ресурсам
// ==========================================
namespace Paths {
    inline constexpr const char* THEMES_DIR       = ":/themes/";
    inline constexpr const char* TRANSLATIONS_DIR = ":/translations/";
    inline constexpr const char* PROFILES_DIR     = "profiles";     // Относительно данных приложения
    inline constexpr const char* MACROS_DIR       = "macros";
    inline constexpr const char* LOGS_DIR         = "logs";
}

// ==========================================
// Размеры GUI
// ==========================================
namespace UI {
    inline constexpr int MAIN_WINDOW_WIDTH    = 750;
    inline constexpr int MAIN_WINDOW_HEIGHT   = 550;
    inline constexpr int MIN_WINDOW_WIDTH     = 600;
    inline constexpr int MIN_WINDOW_HEIGHT    = 400;
    inline constexpr int LOG_MAX_LINES        = 5000;    // Макс. строк в логе
}

} // namespace AppConstants
