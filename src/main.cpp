// ===========================================
// main.cpp — Точка входа AutoClicker Suite
// ===========================================

#include <QApplication>
#include <QIcon>
#include <QDir>

#include "app/MainWindow.h"
#include "app/HotkeyManager.h"
#include "app/SplashScreen.h"
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

    // Показываем splash screen с анимацией
    auto* splash = new SplashScreen();

    // Создаём главное окно, но не показываем его сразу
    MainWindow* mainWindow = new MainWindow();

    // Когда splash завершится — показываем главное окно
    QObject::connect(splash, &SplashScreen::finished, mainWindow, [mainWindow]() {
        mainWindow->show();
    });

    splash->start();

    return app.exec();
}
