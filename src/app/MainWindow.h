#pragma once
// ===========================================
// MainWindow.h — Главное окно приложения
// ===========================================

#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QCloseEvent>
#include <QInputDialog>
#include <QFileDialog>

#include "utils/Logger.h"

// Предварительные объявления
class TrayManager;
class HotkeyManager;
class ThemeManager;
class LanguageManager;
class ProfileManager;

class MouseWidget;
class KeyboardWidget;
class MacroWidget;
class SmartWidget;
class SettingsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    // Перехват закрытия — сворачиваем в трей вместо выхода
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Обработчики меню
    void onThemeToggle();
    void onLanguageToggle();
    void onAbout();
    void onLoadProfile();
    void onSaveProfile();
    void onOpenSettings();

    // Обновление лога в реальном времени
    void onLogMessage(const QString& message, LogLevel level);

    // Статус бар
    void updateStatusBar(const QString& message);

private:
    // Инициализация компонентов
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupLog();
    void setupManagers();
    void connectSignals();
    void restoreWindowState();
    void saveWindowState();
    void updateMacroListForScheduler();

    // ==========================================
    // Вкладки модулей
    // ==========================================
    QTabWidget*      m_tabWidget      = nullptr;
    MouseWidget*     m_mouseWidget    = nullptr;
    KeyboardWidget*  m_keyboardWidget = nullptr;
    MacroWidget*     m_macroWidget    = nullptr;
    SmartWidget*     m_smartWidget    = nullptr;

    // ==========================================
    // Лог-панель
    // ==========================================
    QPlainTextEdit*  m_logView        = nullptr;
    QSplitter*       m_splitter       = nullptr;

    // ==========================================
    // Менеджеры
    // ==========================================
    TrayManager*     m_trayManager    = nullptr;
    HotkeyManager*   m_hotkeyManager  = nullptr;
    ThemeManager*    m_themeManager   = nullptr;
    LanguageManager* m_langManager    = nullptr;
    ProfileManager*  m_profileManager = nullptr;

    // Статус бар
    QLabel*          m_statusLabel    = nullptr;
};
