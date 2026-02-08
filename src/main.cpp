// ===========================================
// main.cpp — Точка входа AutoClicker Suite
// ===========================================

#include <QApplication>
#include <QIcon>
#include <QDir>

#include "app/MainWindow.h"
#include "app/HotkeyManager.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

int main(int argc, char* argv[])
{
    // Включаем High DPI масштабирование
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // Метаданные приложения
    app.setApplicationName(AppConstants::APP_DISPLAY_NAME);
    app.setOrganizationName(AppConstants::APP_ORGANIZATION);
    app.setOrganizationDomain(AppConstants::APP_DOMAIN);
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/icons/app.png"));

    // Не завершать приложение при закрытии последнего окна
    // (остаётся в системном трее)
    app.setQuitOnLastWindowClosed(false);

    // Включаем файловое логирование
    Logger::instance().setFileLoggingEnabled(true);

    LOG_INFO("=== AutoClicker Suite стартует ===");

    // Создаём главное окно
    MainWindow mainWindow;
    mainWindow.show();

    // Регистрируем глобальные хоткеи по умолчанию
    // (HotkeyManager уже создан внутри MainWindow)

    return app.exec();
}
