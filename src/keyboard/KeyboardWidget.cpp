// ===========================================
// KeyboardWidget.cpp — Полная реализация GUI автонажатия клавиатуры
// ===========================================

#include "KeyboardWidget.h"
#include "KeyboardClicker.h"
#include "KeyboardConfig.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QScrollArea>
#include <QApplication>
#include <QKeyEvent>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

KeyboardWidget::KeyboardWidget(QWidget* parent)
    : QWidget(parent)
{
    m_clicker = new KeyboardClicker(this);
    setupUI();
    connectSignals();
    applyConfig();
}

KeyboardWidget::~KeyboardWidget()
{
    if (m_clicker->isRunning()) {
        m_clicker->stop();
    }
    if (m_clicker->isRecording()) {
        m_clicker->stopRecording();
    }
}

// ===========================================
// Построение интерфейса
// ===========================================

void KeyboardWidget::setupUI()
{
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    auto* mainLayout = new QHBoxLayout(scrollContent);

    // ==========================================
    // Левая колонка — режим и настройки
    // ==========================================
    auto* leftColumn = new QVBoxLayout();
    setupModeSelector();
    leftColumn->addWidget(m_modeGroup);
    leftColumn->addWidget(m_modeStack);
    leftColumn->addStretch();

    // ==========================================
    // Правая колонка — ограничения, рандомизация, управление
    // ==========================================
    auto* rightColumn = new QVBoxLayout();
    setupLimitsGroup();
    setupRandomGroup();
    setupControlGroup();
    rightColumn->addWidget(m_limitsGroup);
    rightColumn->addWidget(m_randomGroup);
    rightColumn->addWidget(m_controlGroup);
    rightColumn->addStretch();

    mainLayout->addLayout(leftColumn, 1);
    mainLayout->addLayout(rightColumn, 1);

    scrollArea->setWidget(scrollContent);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
}

// ===========================================
// Выбор режима
// ===========================================

void KeyboardWidget::setupModeSelector()
{
    m_modeGroup = new QGroupBox(tr("Режим работы"), this);
    auto* layout = new QVBoxLayout(m_modeGroup);

    m_modeCombo = new QComboBox();
    m_modeCombo->addItem(tr("⌨ Одиночная клавиша"),  static_cast<int>(KeyboardMode::SingleKey));
    m_modeCombo->addItem(tr("🔗 Комбинация клавиш"), static_cast<int>(KeyboardMode::KeyCombination));
    m_modeCombo->addItem(tr("📝 Ввод текста"),       static_cast<int>(KeyboardMode::TypeText));
    m_modeCombo->addItem(tr("🎬 Макрос клавиатуры"), static_cast<int>(KeyboardMode::MacroPlayback));
    layout->addWidget(m_modeCombo);

    // Стек страниц для каждого режима
    m_modeStack = new QStackedWidget(this);

    setupSingleKeyPage();
    setupComboKeyPage();
    setupTypeTextPage();
    setupMacroPage();

    m_modeStack->addWidget(m_singleKeyPage);
    m_modeStack->addWidget(m_comboKeyPage);
    m_modeStack->addWidget(m_typeTextPage);
    m_modeStack->addWidget(m_macroPage);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyboardWidget::onModeChanged);
}

// ===========================================
// Страница 1: Одиночная клавиша
// ===========================================

void KeyboardWidget::setupSingleKeyPage()
{
    m_singleKeyPage = new QWidget();
    auto* layout = new QFormLayout(m_singleKeyPage);

    // Захват клавиши
    auto* captureLayout = new QHBoxLayout();
    m_captureKeyBtn = new QPushButton(tr("🎯 Захватить клавишу"));
    m_captureKeyBtn->setToolTip(tr("Нажмите, затем нажмите нужную клавишу"));
    m_capturedKeyLabel = new QLabel(tr("Не выбрана"));
    m_capturedKeyLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    captureLayout->addWidget(m_captureKeyBtn);
    captureLayout->addWidget(m_capturedKeyLabel);
    captureLayout->addStretch();
    layout->addRow(tr("Клавиша:"), captureLayout);

    // Интервал
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(AppConstants::Keyboard::MIN_INTERVAL_MS,
                              AppConstants::Keyboard::MAX_INTERVAL_MS);
    m_intervalSpin->setValue(AppConstants::Keyboard::DEFAULT_INTERVAL_MS);
    m_intervalSpin->setSuffix(tr(" мс"));
    m_intervalSpin->setToolTip(tr("Интервал между нажатиями клавиши"));
    layout->addRow(tr("Интервал:"), m_intervalSpin);

    // Модификаторы
    auto* modGroup = new QGroupBox(tr("Модификаторы"));
    auto* modLayout = new QHBoxLayout(modGroup);
    m_modCtrlCheck  = new QCheckBox("Ctrl");
    m_modShiftCheck = new QCheckBox("Shift");
    m_modAltCheck   = new QCheckBox("Alt");
    m_modWinCheck   = new QCheckBox("Win");
    modLayout->addWidget(m_modCtrlCheck);
    modLayout->addWidget(m_modShiftCheck);
    modLayout->addWidget(m_modAltCheck);
    modLayout->addWidget(m_modWinCheck);
    layout->addRow(modGroup);
}

// ===========================================
// Страница 2: Комбинация клавиш
// ===========================================

void KeyboardWidget::setupComboKeyPage()
{
    m_comboKeyPage = new QWidget();
    auto* layout = new QFormLayout(m_comboKeyPage);

    // Захват основной клавиши
    auto* captureLayout = new QHBoxLayout();
    m_captureComboBtn = new QPushButton(tr("🎯 Захватить клавишу"));
    m_capturedComboLabel = new QLabel(tr("Не выбрана"));
    m_capturedComboLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    captureLayout->addWidget(m_captureComboBtn);
    captureLayout->addWidget(m_capturedComboLabel);
    captureLayout->addStretch();
    layout->addRow(tr("Основная клавиша:"), captureLayout);

    // Модификаторы
    auto* modGroup = new QGroupBox(tr("Модификаторы (обязательно выберите хотя бы один)"));
    auto* modLayout = new QHBoxLayout(modGroup);
    m_comboCtrlCheck  = new QCheckBox("Ctrl");
    m_comboCtrlCheck->setChecked(true);  // По умолчанию Ctrl
    m_comboShiftCheck = new QCheckBox("Shift");
    m_comboAltCheck   = new QCheckBox("Alt");
    m_comboWinCheck   = new QCheckBox("Win");
    modLayout->addWidget(m_comboCtrlCheck);
    modLayout->addWidget(m_comboShiftCheck);
    modLayout->addWidget(m_comboAltCheck);
    modLayout->addWidget(m_comboWinCheck);
    layout->addRow(modGroup);

    // Интервал
    m_comboIntervalSpin = new QSpinBox();
    m_comboIntervalSpin->setRange(AppConstants::Keyboard::MIN_INTERVAL_MS,
                                   AppConstants::Keyboard::MAX_INTERVAL_MS);
    m_comboIntervalSpin->setValue(500);  // Комбинации обычно медленнее
    m_comboIntervalSpin->setSuffix(tr(" мс"));
    layout->addRow(tr("Интервал повторения:"), m_comboIntervalSpin);

    // Подсказка
    auto* hint = new QLabel(tr("💡 Пример: Ctrl+V — автоматическая вставка из буфера"));
    hint->setStyleSheet("color: gray; font-size: 9pt;");
    hint->setWordWrap(true);
    layout->addRow(hint);
}

// ===========================================
// Страница 3: Ввод текста
// ===========================================

void KeyboardWidget::setupTypeTextPage()
{
    m_typeTextPage = new QWidget();
    auto* layout = new QVBoxLayout(m_typeTextPage);

    // Поле ввода текста
    auto* textLabel = new QLabel(tr("Текст для ввода:"));
    layout->addWidget(textLabel);

    m_textEdit = new QPlainTextEdit();
    m_textEdit->setPlaceholderText(tr("Введите текст, который будет набран посимвольно..."));
    m_textEdit->setMaximumHeight(120);
    layout->addWidget(m_textEdit);

    // Задержка между символами
    auto* delayLayout = new QHBoxLayout();
    delayLayout->addWidget(new QLabel(tr("Задержка между символами:")));
    m_typeDelaySpin = new QSpinBox();
    m_typeDelaySpin->setRange(1, AppConstants::Keyboard::MAX_TYPE_DELAY);
    m_typeDelaySpin->setValue(AppConstants::Keyboard::DEFAULT_TYPE_DELAY);
    m_typeDelaySpin->setSuffix(tr(" мс"));
    delayLayout->addWidget(m_typeDelaySpin);
    delayLayout->addStretch();
    layout->addLayout(delayLayout);

    // Подсказка
    auto* hint = new QLabel(tr(
        "💡 Текст будет введён посимвольно через Unicode (поддерживает кириллицу, "
        "спецсимволы и эмодзи). Перенос строки — Enter."));
    hint->setStyleSheet("color: gray; font-size: 9pt;");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    layout->addStretch();
}

// ===========================================
// Страница 4: Макрос клавиатуры
// ===========================================

void KeyboardWidget::setupMacroPage()
{
    m_macroPage = new QWidget();
    auto* layout = new QVBoxLayout(m_macroPage);

    // Кнопки управления записью
    auto* btnLayout = new QHBoxLayout();

    m_recordBtn = new QPushButton(tr("⏺ Начать запись"));
    m_recordBtn->setObjectName("recordButton");
    m_recordBtn->setMinimumHeight(36);
    m_recordBtn->setToolTip(tr("Запись нажатий клавиш (F7)"));

    m_clearMacroBtn = new QPushButton(tr("🗑 Очистить"));
    m_clearMacroBtn->setMinimumHeight(36);

    btnLayout->addWidget(m_recordBtn);
    btnLayout->addWidget(m_clearMacroBtn);
    layout->addLayout(btnLayout);

    // Статус записи
    m_macroStatusLabel = new QLabel(tr("Записанных действий: 0"));
    m_macroStatusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_macroStatusLabel);

    // Таблица действий макроса
    m_macroTable = new QTableWidget(0, 3, this);
    m_macroTable->setHorizontalHeaderLabels({tr("Тип"), tr("Клавиша"), tr("Время (мс)")});
    m_macroTable->horizontalHeader()->setStretchLastSection(true);
    m_macroTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_macroTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_macroTable->setMaximumHeight(200);
    layout->addWidget(m_macroTable);

    // Повторений макроса
    auto* repeatLayout = new QHBoxLayout();
    repeatLayout->addWidget(new QLabel(tr("Повторений:")));
    m_macroRepeatSpin = new QSpinBox();
    m_macroRepeatSpin->setRange(0, 100000);
    m_macroRepeatSpin->setValue(1);
    m_macroRepeatSpin->setSpecialValueText(tr("∞ Бесконечно"));
    m_macroRepeatSpin->setToolTip(tr("0 = бесконечный повтор"));
    repeatLayout->addWidget(m_macroRepeatSpin);
    repeatLayout->addStretch();
    layout->addLayout(repeatLayout);
}

// ===========================================
// Ограничения
// ===========================================

void KeyboardWidget::setupLimitsGroup()
{
    m_limitsGroup = new QGroupBox(tr("Ограничения"), this);
    auto* layout = new QVBoxLayout(m_limitsGroup);

    // Лимит по количеству
    auto* countLayout = new QHBoxLayout();
    m_limitCountCheck = new QCheckBox(tr("Макс. нажатий:"));
    m_maxCountSpin = new QSpinBox();
    m_maxCountSpin->setRange(1, 1000000);
    m_maxCountSpin->setValue(100);
    m_maxCountSpin->setEnabled(false);
    countLayout->addWidget(m_limitCountCheck);
    countLayout->addWidget(m_maxCountSpin);
    layout->addLayout(countLayout);

    // Лимит по времени
    auto* timeLayout = new QHBoxLayout();
    m_limitTimeCheck = new QCheckBox(tr("Макс. время:"));
    m_maxTimeSpin = new QSpinBox();
    m_maxTimeSpin->setRange(1, 86400);
    m_maxTimeSpin->setValue(60);
    m_maxTimeSpin->setSuffix(tr(" сек"));
    m_maxTimeSpin->setEnabled(false);
    timeLayout->addWidget(m_limitTimeCheck);
    timeLayout->addWidget(m_maxTimeSpin);
    layout->addLayout(timeLayout);

    connect(m_limitCountCheck, &QCheckBox::toggled, m_maxCountSpin, &QSpinBox::setEnabled);
    connect(m_limitTimeCheck,  &QCheckBox::toggled, m_maxTimeSpin,  &QSpinBox::setEnabled);
}

// ===========================================
// Рандомизация
// ===========================================

void KeyboardWidget::setupRandomGroup()
{
    m_randomGroup = new QGroupBox(tr("Рандомизация (анти-детект)"), this);
    auto* layout = new QVBoxLayout(m_randomGroup);

    m_randomIntervalCheck = new QCheckBox(tr("Случайный интервал"));
    layout->addWidget(m_randomIntervalCheck);

    auto* rangeLayout = new QHBoxLayout();
    rangeLayout->addSpacing(20);
    rangeLayout->addWidget(new QLabel(tr("Мин:")));
    m_randomMinSpin = new QSpinBox();
    m_randomMinSpin->setRange(1, 3600000);
    m_randomMinSpin->setValue(20);
    m_randomMinSpin->setSuffix(tr(" мс"));
    m_randomMinSpin->setEnabled(false);
    rangeLayout->addWidget(m_randomMinSpin);

    rangeLayout->addWidget(new QLabel(tr("Макс:")));
    m_randomMaxSpin = new QSpinBox();
    m_randomMaxSpin->setRange(1, 3600000);
    m_randomMaxSpin->setValue(100);
    m_randomMaxSpin->setSuffix(tr(" мс"));
    m_randomMaxSpin->setEnabled(false);
    rangeLayout->addWidget(m_randomMaxSpin);
    layout->addLayout(rangeLayout);

    connect(m_randomIntervalCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_randomMinSpin->setEnabled(checked);
        m_randomMaxSpin->setEnabled(checked);
    });
}

// ===========================================
// Управление и статус
// ===========================================

void KeyboardWidget::setupControlGroup()
{
    m_controlGroup = new QGroupBox(tr("Управление"), this);
    auto* layout = new QVBoxLayout(m_controlGroup);

    // Кнопки Старт / Стоп
    auto* btnLayout = new QHBoxLayout();

    m_startButton = new QPushButton(tr("▶ Старт"));
    m_startButton->setObjectName("startButton");
    m_startButton->setMinimumHeight(40);
    m_startButton->setToolTip(tr("Начать автонажатие (F6)"));

    m_stopButton = new QPushButton(tr("⏹ Стоп"));
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setMinimumHeight(40);
    m_stopButton->setEnabled(false);
    m_stopButton->setToolTip(tr("Остановить автонажатие (F6)"));

    btnLayout->addWidget(m_startButton);
    btnLayout->addWidget(m_stopButton);
    layout->addLayout(btnLayout);

    // Статус
    m_statusLabel = new QLabel(tr("⏸ Остановлен"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 4px;");
    layout->addWidget(m_statusLabel);

    // Статистика
    auto* statsLayout = new QGridLayout();
    statsLayout->addWidget(new QLabel(tr("Нажатий:")), 0, 0);
    m_pressCountLabel = new QLabel("0");
    m_pressCountLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(m_pressCountLabel, 0, 1);

    statsLayout->addWidget(new QLabel(tr("Время:")), 1, 0);
    m_elapsedLabel = new QLabel("00:00:00");
    m_elapsedLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(m_elapsedLabel, 1, 1);

    layout->addLayout(statsLayout);

    // Подсказка
    auto* hotkeyHint = new QLabel(tr("💡 Горячие клавиши: F6 — Старт/Стоп | F7 — Запись макроса"));
    hotkeyHint->setStyleSheet("color: gray; font-size: 9pt; padding-top: 6px;");
    hotkeyHint->setWordWrap(true);
    layout->addWidget(hotkeyHint);
}

// ===========================================
// Подключение сигналов
// ===========================================

void KeyboardWidget::connectSignals()
{
    // Кнопки управления
    connect(m_startButton, &QPushButton::clicked, this, &KeyboardWidget::onStartClicked);
    connect(m_stopButton,  &QPushButton::clicked, this, &KeyboardWidget::onStopClicked);

    // Сигналы от движка
    connect(m_clicker, &KeyboardClicker::started,      this, &KeyboardWidget::onClickerStarted);
    connect(m_clicker, &KeyboardClicker::stopped,       this, &KeyboardWidget::onClickerStopped);
    connect(m_clicker, &KeyboardClicker::finished,      this, &KeyboardWidget::onClickerFinished);
    connect(m_clicker, &KeyboardClicker::statsUpdated,  this, &KeyboardWidget::onStatsUpdated);

    // Захват клавиши
    connect(m_captureKeyBtn,   &QPushButton::clicked, this, &KeyboardWidget::onCaptureKey);
    connect(m_captureComboBtn, &QPushButton::clicked, this, [this]() {
        m_capturingCombo = true;
        m_captureComboBtn->setText(tr("⏳ Нажмите клавишу..."));
        m_captureComboBtn->setEnabled(false);
        // Устанавливаем фокус для перехвата
        setFocus();
        installEventFilter(this);
    });

    // Запись макроса
    connect(m_recordBtn,    &QPushButton::clicked,   this, &KeyboardWidget::onRecordToggle);
    connect(m_clearMacroBtn, &QPushButton::clicked,   this, &KeyboardWidget::onClearMacro);
    connect(m_clicker, &KeyboardClicker::actionRecorded,
            this, &KeyboardWidget::onActionRecorded);
    connect(m_clicker, &KeyboardClicker::recordingStopped, this, [this]() {
        m_recordBtn->setText(tr("⏺ Начать запись"));
        m_macroStatusLabel->setText(tr("Записанных действий: %1")
            .arg(m_clicker->recordedActions().size()));
        updateMacroTable();
    });

    // Автоприменение конфигурации при изменении
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyboardWidget::applyConfig);
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KeyboardWidget::applyConfig);
    connect(m_comboIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KeyboardWidget::applyConfig);
    connect(m_typeDelaySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &KeyboardWidget::applyConfig);
}

// ===========================================
// Перехват нажатий для захвата клавиши
// ===========================================

bool KeyboardWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        // Игнорируем модификаторы отдельно
        int key = keyEvent->key();
        if (key == Qt::Key_Control || key == Qt::Key_Shift ||
            key == Qt::Key_Alt     || key == Qt::Key_Meta) {
            return true;  // Ждём основную клавишу
        }

#ifdef Q_OS_WIN
        int vkCode = keyEvent->nativeVirtualKey();
#else
        int vkCode = key;
#endif

        if (m_capturingKey) {
            m_capturedVkCode = vkCode;
            m_capturedKeyLabel->setText(KeyboardClicker::vkCodeToName(vkCode));
            m_captureKeyBtn->setText(tr("🎯 Захватить клавишу"));
            m_captureKeyBtn->setEnabled(true);
            m_capturingKey = false;
            removeEventFilter(this);
            applyConfig();
            return true;
        }

        if (m_capturingCombo) {
            m_comboVkCode = vkCode;
            m_capturedComboLabel->setText(KeyboardClicker::vkCodeToName(vkCode));
            m_captureComboBtn->setText(tr("🎯 Захватить клавишу"));
            m_captureComboBtn->setEnabled(true);
            m_capturingCombo = false;
            removeEventFilter(this);
            applyConfig();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

// ===========================================
// Управление
// ===========================================

void KeyboardWidget::toggleStartStop()
{
    if (m_clicker->isRunning()) {
        onStopClicked();
    } else {
        onStartClicked();
    }
}

void KeyboardWidget::onStartClicked()
{
    applyConfig();

    // Валидация перед запуском
    auto mode = static_cast<KeyboardMode>(m_modeCombo->currentData().toInt());
    if (mode == KeyboardMode::SingleKey && m_capturedVkCode == 0) {
        LOG_WARNING(tr("Клавиша не выбрана. Используйте кнопку захвата."));
        return;
    }
    if (mode == KeyboardMode::KeyCombination && m_comboVkCode == 0) {
        LOG_WARNING(tr("Клавиша комбинации не выбрана."));
        return;
    }
    if (mode == KeyboardMode::TypeText && m_textEdit->toPlainText().isEmpty()) {
        LOG_WARNING(tr("Текст для ввода не задан."));
        return;
    }
    if (mode == KeyboardMode::MacroPlayback && m_clicker->recordedActions().isEmpty()
        && m_clicker->config().macroActions.isEmpty()) {
        LOG_WARNING(tr("Нет записанных действий для воспроизведения."));
        return;
    }

    m_clicker->start();
}

void KeyboardWidget::onStopClicked()
{
    m_clicker->stop();
}

// ===========================================
// Применение конфигурации
// ===========================================

void KeyboardWidget::applyConfig()
{
    KeyboardClickerConfig cfg;

    // Режим
    cfg.mode = static_cast<KeyboardMode>(m_modeCombo->currentData().toInt());

    switch (cfg.mode) {
        case KeyboardMode::SingleKey:
            cfg.virtualKeyCode = m_capturedVkCode;
            cfg.intervalMs     = m_intervalSpin->value();
            cfg.withCtrl       = m_modCtrlCheck->isChecked();
            cfg.withShift      = m_modShiftCheck->isChecked();
            cfg.withAlt        = m_modAltCheck->isChecked();
            cfg.withWin        = m_modWinCheck->isChecked();
            break;

        case KeyboardMode::KeyCombination:
            cfg.virtualKeyCode = m_comboVkCode;
            cfg.intervalMs     = m_comboIntervalSpin->value();
            cfg.withCtrl       = m_comboCtrlCheck->isChecked();
            cfg.withShift      = m_comboShiftCheck->isChecked();
            cfg.withAlt        = m_comboAltCheck->isChecked();
            cfg.withWin        = m_comboWinCheck->isChecked();
            break;

        case KeyboardMode::TypeText:
            cfg.textToType     = m_textEdit->toPlainText();
            cfg.typeDelayMs    = m_typeDelaySpin->value();
            break;

        case KeyboardMode::MacroPlayback:
            cfg.macroActions   = m_clicker->recordedActions();
            cfg.repeatCount    = m_macroRepeatSpin->value();
            break;
    }

    // Ограничения (для режимов, кроме макроса, который использует свой repeatSpin)
    if (cfg.mode != KeyboardMode::MacroPlayback) {
        cfg.repeatCount  = m_limitCountCheck->isChecked() ? m_maxCountSpin->value() : 0;
    }
    cfg.timeLimitMs  = m_limitTimeCheck->isChecked() ? m_maxTimeSpin->value() * 1000 : 0;

    // Рандомизация
    cfg.randomizeInterval = m_randomIntervalCheck->isChecked();
    cfg.randomIntervalMin = m_randomMinSpin->value();
    cfg.randomIntervalMax = m_randomMaxSpin->value();

    m_clicker->setConfig(cfg);
}

// ===========================================
// Обработка смены режима
// ===========================================

void KeyboardWidget::onModeChanged(int index)
{
    m_modeStack->setCurrentIndex(index);

    auto mode = static_cast<KeyboardMode>(m_modeCombo->currentData().toInt());

    // Для макроса — лимиты по количеству неприменимы (используется повторения макроса)
    bool showLimits = (mode != KeyboardMode::MacroPlayback);
    m_limitCountCheck->setVisible(showLimits);
    m_maxCountSpin->setVisible(showLimits);
}

// ===========================================
// Захват клавиши
// ===========================================

void KeyboardWidget::onCaptureKey()
{
    m_capturingKey = true;
    m_captureKeyBtn->setText(tr("⏳ Нажмите клавишу..."));
    m_captureKeyBtn->setEnabled(false);
    setFocus();
    installEventFilter(this);
}

// ===========================================
// Запись макроса
// ===========================================

void KeyboardWidget::onRecordToggle()
{
    if (m_clicker->isRecording()) {
        m_clicker->stopRecording();
    } else {
        m_clicker->startRecording();
        m_recordBtn->setText(tr("⏹ Остановить запись"));
        m_macroStatusLabel->setText(tr("🔴 Запись..."));
        m_macroTable->setRowCount(0);
    }
}

void KeyboardWidget::onClearMacro()
{
    m_clicker->clearRecordedActions();
    m_macroTable->setRowCount(0);
    m_macroStatusLabel->setText(tr("Записанных действий: 0"));
    LOG_DEBUG(tr("Клавиатурный макрос очищен"));
}

void KeyboardWidget::onActionRecorded(const KeyboardMacroAction& action)
{
    int row = m_macroTable->rowCount();
    m_macroTable->insertRow(row);

    // Тип действия
    QString typeStr;
    switch (action.type) {
        case KeyboardMacroAction::Type::KeyDown: typeStr = tr("↓ Нажатие");    break;
        case KeyboardMacroAction::Type::KeyUp:   typeStr = tr("↑ Отпускание"); break;
        case KeyboardMacroAction::Type::Delay:   typeStr = tr("⏱ Задержка");   break;
    }
    m_macroTable->setItem(row, 0, new QTableWidgetItem(typeStr));

    // Клавиша или задержка
    if (action.type == KeyboardMacroAction::Type::Delay) {
        m_macroTable->setItem(row, 1, new QTableWidgetItem(tr("%1 мс").arg(action.delayMs)));
    } else {
        m_macroTable->setItem(row, 1,
            new QTableWidgetItem(KeyboardClicker::vkCodeToName(action.vkCode)));
    }

    // Таймстемп
    m_macroTable->setItem(row, 2,
        new QTableWidgetItem(QString::number(action.timestamp)));

    m_macroStatusLabel->setText(tr("🔴 Запись... (действий: %1)").arg(row + 1));

    // Автопрокрутка вниз
    m_macroTable->scrollToBottom();
}

void KeyboardWidget::updateMacroTable()
{
    m_macroTable->setRowCount(0);
    for (const auto& action : m_clicker->recordedActions()) {
        onActionRecorded(action);
    }
    // Обновляем метку после заполнения (onActionRecorded пишет "запись...")
    m_macroStatusLabel->setText(tr("Записанных действий: %1")
        .arg(m_clicker->recordedActions().size()));
}

// ===========================================
// Обновление GUI
// ===========================================

void KeyboardWidget::onClickerStarted()
{
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_statusLabel->setText(tr("🟢 Активен"));
    m_statusLabel->setStyleSheet(
        "font-size: 11pt; font-weight: bold; color: #a6e3a1; padding: 4px;");

    setSettingsEnabled(false);
}

void KeyboardWidget::onClickerStopped()
{
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_statusLabel->setText(tr("⏸ Остановлен"));
    m_statusLabel->setStyleSheet(
        "font-size: 11pt; font-weight: bold; padding: 4px;");

    setSettingsEnabled(true);
}

void KeyboardWidget::onClickerFinished()
{
    m_statusLabel->setText(tr("✅ Завершён (лимит достигнут)"));
    m_statusLabel->setStyleSheet(
        "font-size: 11pt; font-weight: bold; color: #89b4fa; padding: 4px;");
    LOG_INFO(tr("Автонажатие клавиатуры завершено по лимиту"));
}

void KeyboardWidget::onStatsUpdated(qint64 presses, qint64 elapsedMs)
{
    m_pressCountLabel->setText(QString::number(presses));

    int hours   = static_cast<int>(elapsedMs / 3600000);
    int minutes = static_cast<int>((elapsedMs % 3600000) / 60000);
    int seconds = static_cast<int>((elapsedMs % 60000) / 1000);
    m_elapsedLabel->setText(QString("%1:%2:%3")
        .arg(hours,   2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

// ===========================================
// Блокировка/разблокировка настроек
// ===========================================

void KeyboardWidget::setSettingsEnabled(bool enabled)
{
    m_modeGroup->setEnabled(enabled);
    m_modeStack->setEnabled(enabled);
    m_limitsGroup->setEnabled(enabled);
    m_randomGroup->setEnabled(enabled);
}
