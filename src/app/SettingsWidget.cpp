// ===========================================
// SettingsWidget.cpp — Диалог настроек приложения
// ===========================================

#include "SettingsWidget.h"
#include "HotkeyManager.h"
#include "ThemeManager.h"
#include "LanguageManager.h"

#include "utils/Settings.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

SettingsWidget::SettingsWidget(HotkeyManager* hotkeyMgr,
                               ThemeManager* themeMgr,
                               LanguageManager* langMgr,
                               QWidget* parent)
    : QDialog(parent)
    , m_hotkeyMgr(hotkeyMgr)
    , m_themeMgr(themeMgr)
    , m_langMgr(langMgr)
{
    setWindowTitle(tr("Настройки"));
    setMinimumSize(520, 420);
    resize(580, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
    loadSettings();
}

// ===========================================
// Построение интерфейса
// ===========================================

void SettingsWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);

    auto* hotkeyTab = new QWidget();
    setupHotkeyTab(hotkeyTab);
    m_tabs->addTab(hotkeyTab, tr("Горячие клавиши"));

    auto* generalTab = new QWidget();
    setupGeneralTab(generalTab);
    m_tabs->addTab(generalTab, tr("Общие"));

    auto* aboutTab = new QWidget();
    setupAboutTab(aboutTab);
    m_tabs->addTab(aboutTab, tr("О программе"));

    mainLayout->addWidget(m_tabs);

    // Кнопки OK / Применить / Отмена
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_okBtn = new QPushButton(tr("OK"), this);
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, [this]() {
        onApply();
        accept();
    });
    btnLayout->addWidget(m_okBtn);

    m_applyBtn = new QPushButton(tr("Применить"), this);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsWidget::onApply);
    btnLayout->addWidget(m_applyBtn);

    m_cancelBtn = new QPushButton(tr("Отмена"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

// ===========================================
// Вкладка: Горячие клавиши
// ===========================================

void SettingsWidget::setupHotkeyTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);

    auto* group = new QGroupBox(tr("Настройка глобальных горячих клавиш"), tab);
    auto* gridLayout = new QGridLayout(group);

    // Заголовки
    gridLayout->addWidget(new QLabel(tr("Действие"), group), 0, 0);
    gridLayout->addWidget(new QLabel(tr("Комбинация"), group), 0, 1);
    gridLayout->addWidget(new QLabel(tr(""), group), 0, 2);

    // Загружаем текущие биндинги из Settings
    auto& s = Settings::instance();

    // Хоткей 1: Старт/Стоп
    {
        HotkeyBinding hk;
        hk.id = AppConstants::Hotkeys::ID_START_STOP;
        hk.name = tr("Старт / Стоп");
        hk.modifiers = static_cast<UINT>(s.intValue("hotkeys/startStop_mod", 0));
        hk.vk = static_cast<UINT>(s.intValue("hotkeys/startStop_vk", VK_F6));

        hk.keyLabel = new QLabel(hotkeyToString(hk.modifiers, hk.vk), group);
        hk.keyLabel->setMinimumWidth(150);
        hk.keyLabel->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 4px 8px; "
                                    "background: #313244; border: 1px solid #45475a; border-radius: 4px;");

        hk.captureBtn = new QPushButton(tr("Изменить"), group);
        hk.captureBtn->setFixedWidth(100);

        int idx = m_hotkeyBindings.size();
        connect(hk.captureBtn, &QPushButton::clicked, this, [this, idx]() {
            onCaptureHotkey(idx);
        });

        gridLayout->addWidget(new QLabel(hk.name, group), 1, 0);
        gridLayout->addWidget(hk.keyLabel, 1, 1);
        gridLayout->addWidget(hk.captureBtn, 1, 2);

        m_hotkeyBindings.append(hk);
    }

    // Хоткей 2: Запись макроса
    {
        HotkeyBinding hk;
        hk.id = AppConstants::Hotkeys::ID_RECORD_MACRO;
        hk.name = tr("Запись макроса");
        hk.modifiers = static_cast<UINT>(s.intValue("hotkeys/record_mod", 0));
        hk.vk = static_cast<UINT>(s.intValue("hotkeys/record_vk", VK_F7));

        hk.keyLabel = new QLabel(hotkeyToString(hk.modifiers, hk.vk), group);
        hk.keyLabel->setMinimumWidth(150);
        hk.keyLabel->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 4px 8px; "
                                    "background: #313244; border: 1px solid #45475a; border-radius: 4px;");

        hk.captureBtn = new QPushButton(tr("Изменить"), group);
        hk.captureBtn->setFixedWidth(100);

        int idx = m_hotkeyBindings.size();
        connect(hk.captureBtn, &QPushButton::clicked, this, [this, idx]() {
            onCaptureHotkey(idx);
        });

        gridLayout->addWidget(new QLabel(hk.name, group), 2, 0);
        gridLayout->addWidget(hk.keyLabel, 2, 1);
        gridLayout->addWidget(hk.captureBtn, 2, 2);

        m_hotkeyBindings.append(hk);
    }

    // Хоткей 3: Экстренная остановка
    {
        HotkeyBinding hk;
        hk.id = AppConstants::Hotkeys::ID_EMERGENCY_STOP;
        hk.name = tr("Экстренная остановка");
        hk.modifiers = static_cast<UINT>(s.intValue("hotkeys/emergency_mod", 0));
        hk.vk = static_cast<UINT>(s.intValue("hotkeys/emergency_vk", VK_F8));

        hk.keyLabel = new QLabel(hotkeyToString(hk.modifiers, hk.vk), group);
        hk.keyLabel->setMinimumWidth(150);
        hk.keyLabel->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 4px 8px; "
                                    "background: #313244; border: 1px solid #45475a; border-radius: 4px;");

        hk.captureBtn = new QPushButton(tr("Изменить"), group);
        hk.captureBtn->setFixedWidth(100);

        int idx = m_hotkeyBindings.size();
        connect(hk.captureBtn, &QPushButton::clicked, this, [this, idx]() {
            onCaptureHotkey(idx);
        });

        gridLayout->addWidget(new QLabel(hk.name, group), 3, 0);
        gridLayout->addWidget(hk.keyLabel, 3, 1);
        gridLayout->addWidget(hk.captureBtn, 3, 2);

        m_hotkeyBindings.append(hk);
    }

    // Хоткей 4: Выход из приложения
    {
        HotkeyBinding hk;
        hk.id = AppConstants::Hotkeys::ID_EXIT_APP;
        hk.name = tr("Выход из приложения");
        hk.modifiers = static_cast<UINT>(s.intValue("hotkeys/exit_mod", 0));
        hk.vk = static_cast<UINT>(s.intValue("hotkeys/exit_vk", VK_F9));

        hk.keyLabel = new QLabel(hotkeyToString(hk.modifiers, hk.vk), group);
        hk.keyLabel->setMinimumWidth(150);
        hk.keyLabel->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 4px 8px; "
                                    "background: #313244; border: 1px solid #45475a; border-radius: 4px;");

        hk.captureBtn = new QPushButton(tr("Изменить"), group);
        hk.captureBtn->setFixedWidth(100);

        int idx = m_hotkeyBindings.size();
        connect(hk.captureBtn, &QPushButton::clicked, this, [this, idx]() {
            onCaptureHotkey(idx);
        });

        gridLayout->addWidget(new QLabel(hk.name, group), 4, 0);
        gridLayout->addWidget(hk.keyLabel, 4, 1);
        gridLayout->addWidget(hk.captureBtn, 4, 2);

        m_hotkeyBindings.append(hk);
    }

    gridLayout->setColumnStretch(0, 2);
    gridLayout->setColumnStretch(1, 3);
    gridLayout->setColumnStretch(2, 1);

    layout->addWidget(group);

    // Кнопка сброса
    auto* resetLayout = new QHBoxLayout();
    resetLayout->addStretch();
    auto* resetBtn = new QPushButton(tr("Сбросить по умолчанию"), tab);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsWidget::onResetHotkeys);
    resetLayout->addWidget(resetBtn);
    layout->addLayout(resetLayout);

    // Подсказка
    auto* hint = new QLabel(tr("Нажмите \"Изменить\", затем нажмите нужную комбинацию клавиш.\n"
                                "Поддерживаются модификаторы: Ctrl, Shift, Alt.\n"
                                "Нажмите Escape для отмены захвата."), tab);
    hint->setStyleSheet("color: gray; font-size: 9pt; padding: 8px;");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    layout->addStretch();
}

// ===========================================
// Вкладка: Общие
// ===========================================

void SettingsWidget::setupGeneralTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);

    // Поведение
    auto* behaviorGroup = new QGroupBox(tr("Поведение"), tab);
    auto* behaviorLayout = new QVBoxLayout(behaviorGroup);

    m_minimizeToTray = new QCheckBox(tr("Сворачивать в трей при закрытии окна"), behaviorGroup);
    m_minimizeToTray->setChecked(true);
    behaviorLayout->addWidget(m_minimizeToTray);

    m_startMinimized = new QCheckBox(tr("Запускать свёрнуто в трей"), behaviorGroup);
    behaviorLayout->addWidget(m_startMinimized);

    m_confirmExit = new QCheckBox(tr("Подтверждение при выходе"), behaviorGroup);
    behaviorLayout->addWidget(m_confirmExit);

    layout->addWidget(behaviorGroup);

    // Внешний вид
    auto* appearGroup = new QGroupBox(tr("Внешний вид"), tab);
    auto* appearLayout = new QFormLayout(appearGroup);

    m_themeCombo = new QComboBox(appearGroup);
    m_themeCombo->addItem(tr("Тёмная (Catppuccin Mocha)"), "dark");
    m_themeCombo->addItem(tr("Светлая (Catppuccin Latte)"), "light");
    appearLayout->addRow(tr("Тема:"), m_themeCombo);

    m_langCombo = new QComboBox(appearGroup);
    m_langCombo->addItem(tr("Русский"), "ru");
    m_langCombo->addItem(tr("English"), "en");
    appearLayout->addRow(tr("Язык:"), m_langCombo);

    auto* langHint = new QLabel(tr("Изменение языка применяется мгновенно."), appearGroup);
    langHint->setStyleSheet("color: gray; font-size: 9pt;");
    langHint->setWordWrap(true);
    appearLayout->addRow(langHint);

    layout->addWidget(appearGroup);

    // Логирование
    auto* logGroup = new QGroupBox(tr("Логирование"), tab);
    auto* logLayout = new QFormLayout(logGroup);

    m_logMaxLinesSpin = new QSpinBox(logGroup);
    m_logMaxLinesSpin->setRange(100, 50000);
    m_logMaxLinesSpin->setValue(5000);
    m_logMaxLinesSpin->setSuffix(tr(" строк"));
    logLayout->addRow(tr("Макс. строк в логе:"), m_logMaxLinesSpin);

    m_logToFileCb = new QCheckBox(tr("Записывать лог в файл"), logGroup);
    m_logToFileCb->setChecked(true);
    logLayout->addRow(m_logToFileCb);

    layout->addWidget(logGroup);
    layout->addStretch();
}

// ===========================================
// Вкладка: О программе
// ===========================================

void SettingsWidget::setupAboutTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setAlignment(Qt::AlignCenter);

    auto* iconLabel = new QLabel(tab);
    QPixmap icon(":/icons/app.png");
    if (!icon.isNull()) {
        iconLabel->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    auto* titleLabel = new QLabel(
        QString("<h2>AutoClicker Suite v%1</h2>").arg(APP_VERSION), tab);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto* descLabel = new QLabel(
        tr("Продвинутый автокликер для Windows с поддержкой мыши,\n"
           "клавиатуры, макросов и интеллектуальных режимов.\n\n"
           "Разработано с использованием C++20, Qt6, Win32 API.\n\n"
           "Лицензия: MIT"), tab);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: gray;");
    layout->addWidget(descLabel);

    auto* linkLabel = new QLabel(
        QString("<a href='https://github.com/Aramtyla/AutoClickerSuite'>"
                "github.com/Aramtyla/AutoClickerSuite</a>"), tab);
    linkLabel->setAlignment(Qt::AlignCenter);
    linkLabel->setOpenExternalLinks(true);
    layout->addWidget(linkLabel);

    layout->addStretch();
}

// ===========================================
// Загрузка настроек
// ===========================================

void SettingsWidget::loadSettings()
{
    auto& s = Settings::instance();

    // Поведение
    m_minimizeToTray->setChecked(s.boolValue("general/minimizeToTray", true));
    m_startMinimized->setChecked(s.boolValue("general/startMinimized", false));
    m_confirmExit->setChecked(s.boolValue("general/confirmExit", false));

    // Внешний вид
    QString theme = s.stringValue("appearance/theme", "dark");
    m_themeCombo->setCurrentIndex(theme == "light" ? 1 : 0);

    QString lang = s.stringValue("appearance/language", "ru");
    m_langCombo->setCurrentIndex(lang == "en" ? 1 : 0);

    // Логирование
    m_logMaxLinesSpin->setValue(s.intValue("log/maxLines", 5000));
    m_logToFileCb->setChecked(s.boolValue("log/toFile", true));
}

// ===========================================
// Сохранение настроек
// ===========================================

void SettingsWidget::saveSettings()
{
    auto& s = Settings::instance();

    // Поведение
    s.setValue("general/minimizeToTray", m_minimizeToTray->isChecked());
    s.setValue("general/startMinimized", m_startMinimized->isChecked());
    s.setValue("general/confirmExit", m_confirmExit->isChecked());

    // Логирование
    s.setValue("log/maxLines", m_logMaxLinesSpin->value());
    s.setValue("log/toFile", m_logToFileCb->isChecked());
    Logger::instance().setFileLoggingEnabled(m_logToFileCb->isChecked());

    // Горячие клавиши
    for (const auto& hk : m_hotkeyBindings) {
        QString prefix;
        if (hk.id == AppConstants::Hotkeys::ID_START_STOP)    prefix = "hotkeys/startStop";
        if (hk.id == AppConstants::Hotkeys::ID_RECORD_MACRO)  prefix = "hotkeys/record";
        if (hk.id == AppConstants::Hotkeys::ID_EMERGENCY_STOP) prefix = "hotkeys/emergency";
        if (hk.id == AppConstants::Hotkeys::ID_EXIT_APP)       prefix = "hotkeys/exit";

        s.setValue(prefix + "_mod", static_cast<int>(hk.modifiers));
        s.setValue(prefix + "_vk",  static_cast<int>(hk.vk));
    }

    s.sync();
}

// ===========================================
// Применение
// ===========================================

void SettingsWidget::onApply()
{
    // Проверяем, изменился ли язык или тема
    auto& s = Settings::instance();
    QString oldLang = s.stringValue("appearance/language", "ru");
    QString newLang = m_langCombo->currentData().toString();
    QString oldTheme = s.stringValue("appearance/theme", "dark");
    QString newTheme = m_themeCombo->currentData().toString();

    saveSettings();

    // Применяем тему немедленно
    if (oldTheme != newTheme) {
        if (m_themeMgr) {
            m_themeMgr->applyTheme(newTheme == "light" ? AppTheme::Light : AppTheme::Dark);
        }
    }

    // Перерегистрируем хоткеи
    if (m_hotkeyMgr) {
        m_hotkeyMgr->unregisterAll();
        for (const auto& hk : m_hotkeyBindings) {
            m_hotkeyMgr->registerHotkey(hk.id, hk.modifiers, hk.vk);
        }
        emit hotkeysChanged();
    }

    // Если язык изменился — применяем немедленно
    if (oldLang != newLang) {
        if (m_langMgr) {
            m_langMgr->setLanguage(newLang == "en" ? AppLanguage::English : AppLanguage::Russian);
        }
    }

    emit settingsApplied();
    LOG_INFO(tr("Настройки применены"));
}

// ===========================================
// Сброс хоткеев
// ===========================================

void SettingsWidget::onResetHotkeys()
{
    if (m_hotkeyBindings.size() >= 4) {
        m_hotkeyBindings[0].modifiers = 0;
        m_hotkeyBindings[0].vk = VK_F6;
        m_hotkeyBindings[0].keyLabel->setText(hotkeyToString(0, VK_F6));

        m_hotkeyBindings[1].modifiers = 0;
        m_hotkeyBindings[1].vk = VK_F7;
        m_hotkeyBindings[1].keyLabel->setText(hotkeyToString(0, VK_F7));

        m_hotkeyBindings[2].modifiers = 0;
        m_hotkeyBindings[2].vk = VK_F8;
        m_hotkeyBindings[2].keyLabel->setText(hotkeyToString(0, VK_F8));

        m_hotkeyBindings[3].modifiers = 0;
        m_hotkeyBindings[3].vk = VK_F9;
        m_hotkeyBindings[3].keyLabel->setText(hotkeyToString(0, VK_F9));
    }

    LOG_INFO(tr("Горячие клавиши сброшены по умолчанию"));
}

// ===========================================
// Захват горячей клавиши
// ===========================================

void SettingsWidget::onCaptureHotkey(int hotkeyIndex)
{
    if (hotkeyIndex < 0 || hotkeyIndex >= m_hotkeyBindings.size()) return;

    m_capturingIndex = hotkeyIndex;
    m_hotkeyBindings[hotkeyIndex].keyLabel->setText(tr("Нажмите клавишу..."));
    m_hotkeyBindings[hotkeyIndex].keyLabel->setStyleSheet(
        "font-weight: bold; font-size: 11pt; padding: 4px 8px; "
        "background: #fab387; color: #1e1e2e; "
        "border: 1px solid #fab387; border-radius: 4px;");
    m_hotkeyBindings[hotkeyIndex].captureBtn->setEnabled(false);

    // Захватываем фокус
    setFocus();
    grabKeyboard();
}

void SettingsWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_capturingIndex < 0) {
        QDialog::keyPressEvent(event);
        return;
    }

    int key = event->key();

    // Escape — отмена захвата
    if (key == Qt::Key_Escape) {
        releaseKeyboard();
        updateHotkeyLabel(m_capturingIndex);
        m_hotkeyBindings[m_capturingIndex].captureBtn->setEnabled(true);
        m_capturingIndex = -1;
        return;
    }

    // Игнорируем чисто модификаторные нажатия — ждём основную клавишу
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt     || key == Qt::Key_Meta) {
        return;
    }

#ifdef Q_OS_WIN
    UINT vk = event->nativeVirtualKey();
    UINT modifiers = 0;

    if (event->modifiers() & Qt::ControlModifier) modifiers |= MOD_CONTROL;
    if (event->modifiers() & Qt::ShiftModifier)   modifiers |= MOD_SHIFT;
    if (event->modifiers() & Qt::AltModifier)     modifiers |= MOD_ALT;

    m_hotkeyBindings[m_capturingIndex].modifiers = modifiers;
    m_hotkeyBindings[m_capturingIndex].vk = vk;
#endif

    releaseKeyboard();
    updateHotkeyLabel(m_capturingIndex);
    m_hotkeyBindings[m_capturingIndex].captureBtn->setEnabled(true);
    m_capturingIndex = -1;
}

bool SettingsWidget::eventFilter(QObject* obj, QEvent* event)
{
    return QDialog::eventFilter(obj, event);
}

// ===========================================
// Вспомогательные
// ===========================================

QString SettingsWidget::hotkeyToString(UINT modifiers, UINT vk) const
{
    QString result;

#ifdef Q_OS_WIN
    if (modifiers & MOD_CONTROL) result += "Ctrl+";
    if (modifiers & MOD_SHIFT)   result += "Shift+";
    if (modifiers & MOD_ALT)     result += "Alt+";
    if (modifiers & MOD_WIN)     result += "Win+";

    // Получаем имя клавиши через Win32
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= KF_EXTENDED;
            break;
    }

    wchar_t keyName[128] = {};
    int len = GetKeyNameTextW(static_cast<LONG>(scanCode << 16), keyName, 128);
    if (len > 0) {
        result += QString::fromWCharArray(keyName, len);
    } else {
        // Фоллбэк
        switch (vk) {
            case VK_SPACE:  result += "Space"; break;
            case VK_RETURN: result += "Enter"; break;
            case VK_TAB:    result += "Tab"; break;
            case VK_ESCAPE: result += "Escape"; break;
            case VK_BACK:   result += "Backspace"; break;
            default:        result += QString("0x%1").arg(vk, 2, 16, QChar('0')).toUpper(); break;
        }
    }
#else
    result = QString("VK_%1").arg(vk);
#endif

    return result;
}

void SettingsWidget::updateHotkeyLabel(int index)
{
    if (index < 0 || index >= m_hotkeyBindings.size()) return;

    const auto& hk = m_hotkeyBindings[index];
    hk.keyLabel->setText(hotkeyToString(hk.modifiers, hk.vk));
    hk.keyLabel->setStyleSheet(
        "font-weight: bold; font-size: 11pt; padding: 4px 8px; "
        "background: #313244; border: 1px solid #45475a; border-radius: 4px;");
}
