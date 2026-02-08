// ===========================================
// MainWindow.cpp — Реализация главного окна
// ===========================================

#include "MainWindow.h"
#include "TrayManager.h"
#include "HotkeyManager.h"
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"

#include "mouse/MouseWidget.h"
#include "mouse/MouseClicker.h"
#include "keyboard/KeyboardWidget.h"
#include "keyboard/KeyboardClicker.h"
#include "macro/MacroWidget.h"
#include "macro/MacroRecorder.h"
#include "macro/MacroPlayer.h"
#include "smart/SmartWidget.h"
#include "smart/Scheduler.h"

#include "utils/Logger.h"
#include "utils/Settings.h"
#include "utils/Constants.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QVBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QStandardPaths>
#include <QDir>
#include <QClipboard>
#include <QMenu>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Устанавливаем основные параметры окна
    setWindowTitle(tr("AutoClicker Suite"));
    setMinimumSize(AppConstants::UI::MIN_WINDOW_WIDTH,
                   AppConstants::UI::MIN_WINDOW_HEIGHT);
    resize(AppConstants::UI::MAIN_WINDOW_WIDTH,
           AppConstants::UI::MAIN_WINDOW_HEIGHT);

    // Инициализация в правильном порядке
    setupManagers();
    setupUI();
    setupMenuBar();
    setupStatusBar();
    connectSignals();

    // Передаём ссылки на виджеты в ProfileManager
    m_profileManager->setModuleWidgets(m_mouseWidget, m_keyboardWidget, m_smartWidget);

    // Передаём список макросов в SmartWidget для планировщика
    updateMacroListForScheduler();

    restoreWindowState();

    LOG_INFO(tr("AutoClicker Suite запущен (v%1)").arg(APP_VERSION));
    updateStatusBar(tr("Готов к работе"));
}

MainWindow::~MainWindow()
{
    saveWindowState();
}

// ===========================================
// Построение интерфейса
// ===========================================

void MainWindow::setupUI()
{
    // Центральный виджет — сплиттер (вкладки сверху, лог снизу)
    m_splitter = new QSplitter(Qt::Vertical, this);

    // ==========================================
    // Вкладки модулей
    // ==========================================
    m_tabWidget = new QTabWidget(m_splitter);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setDocumentMode(true);

    // Создаём виджеты модулей
    m_mouseWidget    = new MouseWidget(m_tabWidget);
    m_keyboardWidget = new KeyboardWidget(m_tabWidget);
    m_macroWidget    = new MacroWidget(m_tabWidget);
    m_smartWidget    = new SmartWidget(m_tabWidget);

    // Добавляем вкладки
    m_tabWidget->addTab(m_mouseWidget,    QIcon(":/icons/mouse.png"),    tr("Мышь"));
    m_tabWidget->addTab(m_keyboardWidget, QIcon(":/icons/keyboard.png"), tr("Клавиатура"));
    m_tabWidget->addTab(m_macroWidget,    QIcon(":/icons/macro.png"),    tr("Макросы"));
    m_tabWidget->addTab(m_smartWidget,    QIcon(":/icons/smart.png"),    tr("Умные режимы"));

    // ==========================================
    // Лог-панель внизу
    // ==========================================
    setupLog();

    // Сплиттер: 70% вкладки, 30% лог
    m_splitter->addWidget(m_tabWidget);
    m_splitter->addWidget(m_logView);
    m_splitter->setStretchFactor(0, 7);
    m_splitter->setStretchFactor(1, 3);

    setCentralWidget(m_splitter);
}

void MainWindow::setupLog()
{
    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(AppConstants::UI::LOG_MAX_LINES);
    m_logView->setPlaceholderText(tr("Лог действий..."));

    // Моноширинный шрифт для лога
    QFont logFont("Consolas", 9);
    logFont.setStyleHint(QFont::Monospace);
    m_logView->setFont(logFont);

    // Контекстное меню лога
    m_logView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_logView, &QPlainTextEdit::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(tr("Очистить лог"), m_logView, &QPlainTextEdit::clear);
        menu.addAction(tr("Копировать лог"), this, [this]() {
            QApplication::clipboard()->setText(m_logView->toPlainText());
        });
        menu.addAction(tr("Сохранить лог в файл..."), this, [this]() {
            QString path = QFileDialog::getSaveFileName(this, tr("Сохранить лог"),
                QString(), tr("Текстовые файлы (*.txt);;Все файлы (*)"));
            if (!path.isEmpty()) {
                QFile file(path);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(m_logView->toPlainText().toUtf8());
                    file.close();
                    LOG_INFO(tr("Лог сохранён: %1").arg(path));
                }
            }
        });
        menu.exec(m_logView->mapToGlobal(pos));
    });
}

void MainWindow::setupMenuBar()
{
    // ==========================================
    // Меню "Файл"
    // ==========================================
    QMenu* fileMenu = menuBar()->addMenu(tr("&Файл"));

    QAction* loadProfileAction = fileMenu->addAction(tr("Загрузить профиль..."));
    connect(loadProfileAction, &QAction::triggered, this, &MainWindow::onLoadProfile);

    QAction* saveProfileAction = fileMenu->addAction(tr("Сохранить профиль..."));
    connect(saveProfileAction, &QAction::triggered, this, &MainWindow::onSaveProfile);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction(tr("&Выход"));
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // ==========================================
    // Меню "Вид"
    // ==========================================
    QMenu* viewMenu = menuBar()->addMenu(tr("&Вид"));

    QAction* themeAction = viewMenu->addAction(tr("Переключить тему"));
    themeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onThemeToggle);

    QAction* langAction = viewMenu->addAction(tr("Сменить язык (RU/EN)"));
    langAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(langAction, &QAction::triggered, this, &MainWindow::onLanguageToggle);

    // ==========================================
    // Меню "Справка"
    // ==========================================
    QMenu* helpMenu = menuBar()->addMenu(tr("&Справка"));

    QAction* aboutAction = helpMenu->addAction(tr("О программе"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Готов"), this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::setupManagers()
{
    // Менеджер тем
    m_themeManager = new ThemeManager(this);

    // Менеджер языков
    m_langManager = new LanguageManager(this);

    // Менеджер трея
    m_trayManager = new TrayManager(this);

    // Менеджер глобальных хоткеев
    m_hotkeyManager = new HotkeyManager(this);

    // Менеджер профилей
    m_profileManager = new ProfileManager(this);
}

void MainWindow::connectSignals()
{
    // Подключаем логгер к GUI-панели
    connect(&Logger::instance(), &Logger::logAdded,
            this, &MainWindow::onLogMessage);

    // Трей — двойной клик показывает окно
    connect(m_trayManager, &TrayManager::showWindowRequested, this, [this]() {
        showNormal();
        activateWindow();
        raise();
    });

    // Трей — выход
    connect(m_trayManager, &TrayManager::quitRequested, qApp, &QApplication::quit);

    // Трей — горячие действия (дублируют хоткеи)
    connect(m_trayManager, &TrayManager::startStopTriggered, m_hotkeyManager, &HotkeyManager::startStopTriggered);
    connect(m_trayManager, &TrayManager::recordMacroTriggered, m_hotkeyManager, &HotkeyManager::recordMacroTriggered);
    connect(m_trayManager, &TrayManager::emergencyStopTriggered, m_hotkeyManager, &HotkeyManager::emergencyStopTriggered);

    // ==========================================
    // Глобальные хоткеи → модули
    // ==========================================
    m_hotkeyManager->registerDefaults();

    // F6 — Старт/Стоп активного модуля (зависит от текущей вкладки)
    connect(m_hotkeyManager, &HotkeyManager::startStopTriggered, this, [this]() {
        int tab = m_tabWidget->currentIndex();
        if (tab == 0) {
            m_mouseWidget->toggleStartStop();
        } else if (tab == 1) {
            m_keyboardWidget->toggleStartStop();
        } else if (tab == 2) {
            m_macroWidget->toggleStartStop();
        }
    });

    // F7 — Запись макроса (переключается между клавиатурным и полноценным макросом)
    connect(m_hotkeyManager, &HotkeyManager::recordMacroTriggered, this, [this]() {
        int tab = m_tabWidget->currentIndex();
        if (tab == 2) {
            // На вкладке макросов — полная запись (мышь + клавиатура)
            m_macroWidget->toggleRecord();
            if (m_macroWidget->recorder()->isRecording()) {
                updateStatusBar(tr("🔴 Запись макроса..."));
            } else {
                updateStatusBar(tr("Запись макроса остановлена"));
            }
        } else {
            // На других вкладках — запись клавиатурного макроса
            if (m_keyboardWidget->clicker()->isRecording()) {
                m_keyboardWidget->clicker()->stopRecording();
                updateStatusBar(tr("Запись макроса остановлена"));
            } else {
                m_keyboardWidget->clicker()->startRecording();
                updateStatusBar(tr("🔴 Запись макроса..."));
            }
        }
    });

    // F8 — Экстренная остановка всех модулей
    connect(m_hotkeyManager, &HotkeyManager::emergencyStopTriggered, this, [this]() {
        if (m_mouseWidget->clicker()->isRunning()) {
            m_mouseWidget->toggleStartStop();
        }
        if (m_keyboardWidget->clicker()->isRunning()) {
            m_keyboardWidget->toggleStartStop();
        }
        if (m_keyboardWidget->clicker()->isRecording()) {
            m_keyboardWidget->clicker()->stopRecording();
        }
        // Останавливаем макро-движок
        m_macroWidget->emergencyStop();

        // Останавливаем умные режимы
        m_smartWidget->emergencyStop();

        updateStatusBar(tr("⛔ Экстренная остановка!"));
        LOG_WARNING(tr("Экстренная остановка всех модулей (F8)"));
    });

    // Планировщик → запуск макроса
    connect(m_smartWidget, &SmartWidget::runMacroRequested, this, [this](const QString& macroName) {
        m_tabWidget->setCurrentIndex(2);
        LOG_INFO(tr("Планировщик: запуск макроса '%1'").arg(macroName));
        if (!m_macroWidget->playMacroByName(macroName)) {
            LOG_WARNING(tr("Макрос '%1' не найден в библиотеке").arg(macroName));
        }
    });
}

// ===========================================
// Обработчики событий
// ===========================================

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Сворачиваем в трей вместо закрытия
    if (m_trayManager && m_trayManager->isVisible()) {
        hide();
        m_trayManager->showMessage(tr("AutoClicker Suite"),
                                    tr("Приложение свёрнуто в трей. "
                                       "Двойной клик — открыть. "
                                       "Глобальные хоткеи продолжают работать."));
        event->ignore();
    } else {
        saveWindowState();
        event->accept();
    }
}

// ===========================================
// Слоты меню
// ===========================================

void MainWindow::onThemeToggle()
{
    m_themeManager->toggleTheme();
    LOG_INFO(tr("Тема изменена на: %1").arg(m_themeManager->currentThemeName()));
}

void MainWindow::onLanguageToggle()
{
    m_langManager->toggleLanguage();
    LOG_INFO(tr("Язык изменён на: %1").arg(m_langManager->currentLanguageName()));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("О программе"),
        tr("<h3>AutoClicker Suite v%1</h3>"
           "<p>Продвинутый автокликер с поддержкой мыши, клавиатуры, "
           "макросов и умных режимов.</p>"
           "<p>C++ / Qt6 / Win32 API</p>")
        .arg(APP_VERSION));
}

void MainWindow::onLoadProfile()
{
    QStringList profiles = m_profileManager->availableProfiles();
    if (profiles.isEmpty()) {
        QMessageBox::information(this, tr("Профили"),
            tr("Нет сохранённых профилей."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getItem(this, tr("Загрузить профиль"),
        tr("Выберите профиль:"), profiles, 0, false, &ok);

    if (ok && !name.isEmpty()) {
        if (m_profileManager->loadProfile(name)) {
            updateStatusBar(tr("Профиль загружен: %1").arg(name));
        } else {
            QMessageBox::warning(this, tr("Ошибка"),
                tr("Не удалось загрузить профиль: %1").arg(name));
        }
    }
}

void MainWindow::onSaveProfile()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Сохранить профиль"),
        tr("Имя профиля:"), QLineEdit::Normal,
        m_profileManager->currentProfile(), &ok);

    if (ok && !name.isEmpty()) {
        if (m_profileManager->saveProfile(name)) {
            updateStatusBar(tr("Профиль сохранён: %1").arg(name));
        } else {
            QMessageBox::warning(this, tr("Ошибка"),
                tr("Не удалось сохранить профиль: %1").arg(name));
        }
    }
}

// ===========================================
// Лог
// ===========================================

void MainWindow::onLogMessage(const QString& message, LogLevel level)
{
    if (!m_logView) return;

    // Цветовое выделение по уровню, адаптивное к теме
    bool isDark = m_themeManager && m_themeManager->currentTheme() == AppTheme::Dark;
    QString color;
    switch (level) {
        case LogLevel::Debug:   color = isDark ? "#6c7086" : "#9ca0b0"; break;  // серый
        case LogLevel::Info:    color = isDark ? "#cdd6f4" : "#4c4f69"; break;  // основной текст
        case LogLevel::Warning: color = isDark ? "#fab387" : "#fe640b"; break;  // оранжевый
        case LogLevel::Error:   color = isDark ? "#f38ba8" : "#d20f39"; break;  // красный
    }

    m_logView->appendHtml(
        QString("<span style='color:%1;'>%2</span>").arg(color, message.toHtmlEscaped()));
}

void MainWindow::updateStatusBar(const QString& message)
{
    if (m_statusLabel) {
        m_statusLabel->setText(message);
    }
}

// ===========================================
// Сохранение/восстановление состояния окна
// ===========================================

void MainWindow::restoreWindowState()
{
    auto& s = Settings::instance();
    if (s.contains("window/geometry")) {
        restoreGeometry(s.value("window/geometry").toByteArray());
    }
    if (s.contains("window/state")) {
        restoreState(s.value("window/state").toByteArray());
    }
    if (s.contains("window/splitter")) {
        m_splitter->restoreState(s.value("window/splitter").toByteArray());
    }
}

void MainWindow::saveWindowState()
{
    auto& s = Settings::instance();
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state", saveState());
    s.setValue("window/splitter", m_splitter->saveState());
    s.sync();
}

// ==========================================
// Обновление списка макросов для планировщика
// ==========================================

void MainWindow::updateMacroListForScheduler()
{
    // Собираем имена макросов из MacroWidget
    // MacroWidget хранит m_macros как QMap<QString, Macro>
    // Используем публичный API — получаем через виджет списка
    QStringList macroNames;

    // Перебираем элементы списка QListWidget в MacroWidget
    // Вместо прямого доступа — сканируем каталог макросов
    QString macroDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + "/" + AppConstants::Paths::MACROS_DIR;
    QDir dir(macroDir);
    if (dir.exists()) {
        QStringList filters = {"*.json"};
        QStringList files = dir.entryList(filters, QDir::Files);
        for (const QString& f : files) {
            macroNames.append(f.chopped(5)); // убираем ".json"
        }
    }

    m_smartWidget->setAvailableMacros(macroNames);
}
