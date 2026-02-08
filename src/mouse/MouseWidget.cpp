// ===========================================
// MouseWidget.cpp — Полная реализация GUI автоклика мыши
// ===========================================

#include "MouseWidget.h"
#include "MouseClicker.h"
#include "MouseConfig.h"
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
#include <QCursor>
#include <QApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

MouseWidget::MouseWidget(QWidget* parent)
    : QWidget(parent)
{
    m_clicker = new MouseClicker(this);
    setupUI();
    connectSignals();
    applyConfig();  // Применяем начальную конфигурацию
}

MouseWidget::~MouseWidget()
{
    if (m_clicker->isRunning()) {
        m_clicker->stop();
    }
}

// ===========================================
// Построение интерфейса
// ===========================================

void MouseWidget::setupUI()
{
    // Основной скроллируемый виджет (на случай малого экрана)
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    auto* mainLayout = new QHBoxLayout(scrollContent);

    // ==========================================
    // Левая колонка — настройки
    // ==========================================
    auto* leftColumn = new QVBoxLayout();
    setupClickSettingsGroup();
    setupPositionGroup();
    setupMultiPointGroup();
    leftColumn->addWidget(m_clickGroup);
    leftColumn->addWidget(m_posGroup);
    leftColumn->addWidget(m_multiGroup);
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

    // Таймер обновления позиции курсора
    m_posUpdateTimer = new QTimer(this);
    m_posUpdateTimer->setInterval(100);
    connect(m_posUpdateTimer, &QTimer::timeout, this, &MouseWidget::updatePositionLabel);
    m_posUpdateTimer->start();
}

void MouseWidget::setupClickSettingsGroup()
{
    m_clickGroup = new QGroupBox(tr("Настройки клика"), this);
    auto* layout = new QFormLayout(m_clickGroup);

    // Кнопка мыши
    m_buttonCombo = new QComboBox();
    m_buttonCombo->addItem(tr("Левая кнопка (ЛКМ)"),  static_cast<int>(MouseButton::Left));
    m_buttonCombo->addItem(tr("Правая кнопка (ПКМ)"),  static_cast<int>(MouseButton::Right));
    m_buttonCombo->addItem(tr("Средняя кнопка (СКМ)"), static_cast<int>(MouseButton::Middle));
    layout->addRow(tr("Кнопка:"), m_buttonCombo);

    // Тип клика
    m_clickTypeCombo = new QComboBox();
    m_clickTypeCombo->addItem(tr("Одиночный клик"),  static_cast<int>(ClickType::Single));
    m_clickTypeCombo->addItem(tr("Двойной клик"),    static_cast<int>(ClickType::Double));
    m_clickTypeCombo->addItem(tr("Удержание"),       static_cast<int>(ClickType::Hold));
    layout->addRow(tr("Тип клика:"), m_clickTypeCombo);

    // Интервал
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(AppConstants::Mouse::MIN_INTERVAL_MS,
                              AppConstants::Mouse::MAX_INTERVAL_MS);
    m_intervalSpin->setValue(AppConstants::Mouse::DEFAULT_INTERVAL_MS);
    m_intervalSpin->setSuffix(tr(" мс"));
    m_intervalSpin->setToolTip(tr("Интервал между кликами в миллисекундах"));
    layout->addRow(tr("Интервал:"), m_intervalSpin);

    // Длительность удержания
    m_holdDurationSpin = new QSpinBox();
    m_holdDurationSpin->setRange(10, 60000);
    m_holdDurationSpin->setValue(100);
    m_holdDurationSpin->setSuffix(tr(" мс"));
    m_holdDurationSpin->setEnabled(false);
    layout->addRow(tr("Длительность удержания:"), m_holdDurationSpin);

    // Включаем/выключаем поле удержания по типу клика
    connect(m_clickTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        m_holdDurationSpin->setEnabled(index == static_cast<int>(ClickType::Hold));
    });
}

void MouseWidget::setupPositionGroup()
{
    m_posGroup = new QGroupBox(tr("Позиция клика"), this);
    auto* layout = new QVBoxLayout(m_posGroup);

    // Радиокнопки режимов
    m_followCursorRadio = new QRadioButton(tr("Следовать за курсором"));
    m_followCursorRadio->setChecked(true);
    m_followCursorRadio->setToolTip(tr("Клик в текущей позиции курсора"));

    m_fixedPosRadio = new QRadioButton(tr("Фиксированная позиция"));
    m_fixedPosRadio->setToolTip(tr("Клик по указанным координатам"));

    m_multiPointRadio = new QRadioButton(tr("Мультиточечный режим"));
    m_multiPointRadio->setToolTip(tr("Последовательный клик по списку координат"));

    layout->addWidget(m_followCursorRadio);
    layout->addWidget(m_fixedPosRadio);

    // Координаты фиксированной позиции
    auto* coordLayout = new QHBoxLayout();
    coordLayout->addSpacing(20);
    coordLayout->addWidget(new QLabel("X:"));

    m_fixedXSpin = new QSpinBox();
    m_fixedXSpin->setRange(0, 9999);
    m_fixedXSpin->setEnabled(false);
    coordLayout->addWidget(m_fixedXSpin);

    coordLayout->addWidget(new QLabel("Y:"));
    m_fixedYSpin = new QSpinBox();
    m_fixedYSpin->setRange(0, 9999);
    m_fixedYSpin->setEnabled(false);
    coordLayout->addWidget(m_fixedYSpin);

    m_pickPosButton = new QPushButton(tr("Захватить"));
    m_pickPosButton->setToolTip(tr("Нажмите, затем кликните мышью для захвата координат (5 сек)"));
    m_pickPosButton->setEnabled(false);
    coordLayout->addWidget(m_pickPosButton);

    layout->addLayout(coordLayout);
    layout->addWidget(m_multiPointRadio);

    // Текущая позиция курсора
    m_currentPosLabel = new QLabel(tr("Курсор: (0, 0)"));
    m_currentPosLabel->setStyleSheet("color: gray; font-size: 9pt;");
    layout->addWidget(m_currentPosLabel);

    // Активация/деактивация полей координат
    connect(m_fixedPosRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_fixedXSpin->setEnabled(checked);
        m_fixedYSpin->setEnabled(checked);
        m_pickPosButton->setEnabled(checked);
    });

    connect(m_multiPointRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_multiGroup->setVisible(checked);
    });
}

void MouseWidget::setupMultiPointGroup()
{
    m_multiGroup = new QGroupBox(tr("Мультиточечный маршрут"), this);
    m_multiGroup->setVisible(false);  // Скрыт по умолчанию
    auto* layout = new QVBoxLayout(m_multiGroup);

    // Таблица точек
    m_pointsTable = new QTableWidget(0, 3, this);
    m_pointsTable->setHorizontalHeaderLabels({tr("X"), tr("Y"), tr("Задержка (мс)")});
    m_pointsTable->horizontalHeader()->setStretchLastSection(true);
    m_pointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pointsTable->setMaximumHeight(150);
    layout->addWidget(m_pointsTable);

    // Кнопки управления
    auto* btnLayout = new QHBoxLayout();

    m_addPointBtn = new QPushButton(tr("+ Добавить точку"));
    m_removePointBtn = new QPushButton(tr("- Удалить"));
    m_clearPointsBtn = new QPushButton(tr("Очистить"));

    btnLayout->addWidget(m_addPointBtn);
    btnLayout->addWidget(m_removePointBtn);
    btnLayout->addWidget(m_clearPointsBtn);
    layout->addLayout(btnLayout);
}

void MouseWidget::setupLimitsGroup()
{
    m_limitsGroup = new QGroupBox(tr("Ограничения"), this);
    auto* layout = new QVBoxLayout(m_limitsGroup);

    // Ограничение по кликам
    auto* clickLimitLayout = new QHBoxLayout();
    m_limitClicksCheck = new QCheckBox(tr("Макс. кликов:"));
    m_maxClicksSpin = new QSpinBox();
    m_maxClicksSpin->setRange(1, AppConstants::Mouse::MAX_CLICK_COUNT);
    m_maxClicksSpin->setValue(100);
    m_maxClicksSpin->setEnabled(false);
    clickLimitLayout->addWidget(m_limitClicksCheck);
    clickLimitLayout->addWidget(m_maxClicksSpin);
    layout->addLayout(clickLimitLayout);

    // Ограничение по времени
    auto* timeLimitLayout = new QHBoxLayout();
    m_limitTimeCheck = new QCheckBox(tr("Макс. время:"));
    m_maxTimeSpin = new QSpinBox();
    m_maxTimeSpin->setRange(1, 86400);  // Макс. 24 часа
    m_maxTimeSpin->setValue(60);
    m_maxTimeSpin->setSuffix(tr(" сек"));
    m_maxTimeSpin->setEnabled(false);
    timeLimitLayout->addWidget(m_limitTimeCheck);
    timeLimitLayout->addWidget(m_maxTimeSpin);
    layout->addLayout(timeLimitLayout);

    // Связываем чекбоксы с активностью спинбоксов
    connect(m_limitClicksCheck, &QCheckBox::toggled, m_maxClicksSpin, &QSpinBox::setEnabled);
    connect(m_limitTimeCheck,   &QCheckBox::toggled, m_maxTimeSpin,   &QSpinBox::setEnabled);
}

void MouseWidget::setupRandomGroup()
{
    m_randomGroup = new QGroupBox(tr("Рандомизация (анти-детект)"), this);
    auto* layout = new QVBoxLayout(m_randomGroup);

    // Случайный интервал
    m_randomIntervalCheck = new QCheckBox(tr("Случайный интервал"));
    layout->addWidget(m_randomIntervalCheck);

    auto* intervalRange = new QHBoxLayout();
    intervalRange->addSpacing(20);
    intervalRange->addWidget(new QLabel(tr("Мин:")));
    m_randomMinSpin = new QSpinBox();
    m_randomMinSpin->setRange(1, 3600000);
    m_randomMinSpin->setValue(50);
    m_randomMinSpin->setSuffix(tr(" мс"));
    m_randomMinSpin->setEnabled(false);
    intervalRange->addWidget(m_randomMinSpin);

    intervalRange->addWidget(new QLabel(tr("Макс:")));
    m_randomMaxSpin = new QSpinBox();
    m_randomMaxSpin->setRange(1, 3600000);
    m_randomMaxSpin->setValue(200);
    m_randomMaxSpin->setSuffix(tr(" мс"));
    m_randomMaxSpin->setEnabled(false);
    intervalRange->addWidget(m_randomMaxSpin);
    layout->addLayout(intervalRange);

    // Случайное смещение позиции
    m_randomPosCheck = new QCheckBox(tr("Случайное смещение позиции"));
    layout->addWidget(m_randomPosCheck);

    auto* offsetLayout = new QHBoxLayout();
    offsetLayout->addSpacing(20);
    offsetLayout->addWidget(new QLabel(tr("Радиус:")));
    m_randomOffsetSpin = new QSpinBox();
    m_randomOffsetSpin->setRange(1, AppConstants::Mouse::RANDOM_OFFSET_MAX);
    m_randomOffsetSpin->setValue(5);
    m_randomOffsetSpin->setSuffix(tr(" пикс."));
    m_randomOffsetSpin->setEnabled(false);
    offsetLayout->addWidget(m_randomOffsetSpin);
    offsetLayout->addStretch();
    layout->addLayout(offsetLayout);

    // Связываем чекбоксы
    connect(m_randomIntervalCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_randomMinSpin->setEnabled(checked);
        m_randomMaxSpin->setEnabled(checked);
    });
    connect(m_randomPosCheck, &QCheckBox::toggled, m_randomOffsetSpin, &QSpinBox::setEnabled);
}

void MouseWidget::setupControlGroup()
{
    auto* group = new QGroupBox(tr("Управление"), this);
    m_controlGroup = group;

    auto* layout = new QVBoxLayout(group);

    // Кнопки Старт/Стоп
    auto* btnLayout = new QHBoxLayout();

    m_startButton = new QPushButton(tr("Старт"));
    m_startButton->setObjectName("startButton");
    m_startButton->setMinimumHeight(40);
    m_startButton->setToolTip(tr("Начать автоклик (F6)"));

    m_stopButton = new QPushButton(tr("Стоп"));
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setMinimumHeight(40);
    m_stopButton->setEnabled(false);
    m_stopButton->setToolTip(tr("Остановить автоклик (F6)"));

    btnLayout->addWidget(m_startButton);
    btnLayout->addWidget(m_stopButton);
    layout->addLayout(btnLayout);

    // Статус
    m_statusLabel = new QLabel(tr("Остановлен"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 4px;");
    layout->addWidget(m_statusLabel);

    // Статистика
    auto* statsLayout = new QGridLayout();
    statsLayout->addWidget(new QLabel(tr("Кликов:")), 0, 0);
    m_clickCountLabel = new QLabel("0");
    m_clickCountLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(m_clickCountLabel, 0, 1);

    statsLayout->addWidget(new QLabel(tr("Время:")), 1, 0);
    m_elapsedLabel = new QLabel("00:00:00");
    m_elapsedLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(m_elapsedLabel, 1, 1);

    layout->addLayout(statsLayout);

    // Подсказка по хоткею
    auto* hotkeyHint = new QLabel(tr("Горячая клавиша: F6 — Старт/Стоп"));
    hotkeyHint->setStyleSheet("color: gray; font-size: 9pt; padding-top: 6px;");
    hotkeyHint->setWordWrap(true);
    layout->addWidget(hotkeyHint);
}

void MouseWidget::connectSignals()
{
    // Кнопки управления
    connect(m_startButton, &QPushButton::clicked, this, &MouseWidget::onStartClicked);
    connect(m_stopButton,  &QPushButton::clicked, this, &MouseWidget::onStopClicked);

    // Сигналы от кликера
    connect(m_clicker, &MouseClicker::started,      this, &MouseWidget::onClickerStarted);
    connect(m_clicker, &MouseClicker::stopped,       this, &MouseWidget::onClickerStopped);
    connect(m_clicker, &MouseClicker::finished,      this, &MouseWidget::onClickerFinished);
    connect(m_clicker, &MouseClicker::statsUpdated,  this, &MouseWidget::onStatsUpdated);

    // Кнопки мультиточечного режима
    connect(m_addPointBtn,    &QPushButton::clicked, this, &MouseWidget::onAddPoint);
    connect(m_removePointBtn, &QPushButton::clicked, this, &MouseWidget::onRemovePoint);
    connect(m_clearPointsBtn, &QPushButton::clicked, this, &MouseWidget::onClearPoints);
    connect(m_pickPosButton,  &QPushButton::clicked, this, &MouseWidget::onPickPosition);

    // Автоприменение конфигурации при изменении настроек
    connect(m_buttonCombo,    QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MouseWidget::applyConfig);
    connect(m_clickTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MouseWidget::applyConfig);
    connect(m_intervalSpin,   QOverload<int>::of(&QSpinBox::valueChanged),         this, &MouseWidget::applyConfig);
}

// ===========================================
// Управление
// ===========================================

void MouseWidget::toggleStartStop()
{
    if (m_clicker->isRunning()) {
        onStopClicked();
    } else {
        onStartClicked();
    }
}

void MouseWidget::onStartClicked()
{
    applyConfig();
    m_clicker->start();
}

void MouseWidget::onStopClicked()
{
    m_clicker->stop();
}

// ===========================================
// Применение конфигурации
// ===========================================

void MouseWidget::applyConfig()
{
    MouseClickerConfig cfg;

    // Кнопка мыши
    cfg.button = static_cast<MouseButton>(m_buttonCombo->currentData().toInt());

    // Тип клика
    cfg.clickType = static_cast<ClickType>(m_clickTypeCombo->currentData().toInt());

    // Интервал
    cfg.intervalMs = m_intervalSpin->value();

    // Длительность удержания
    cfg.holdDurationMs = m_holdDurationSpin->value();

    // Позиция
    if (m_followCursorRadio->isChecked()) {
        cfg.positionMode = ClickPosition::FollowCursor;
    } else if (m_fixedPosRadio->isChecked()) {
        cfg.positionMode = ClickPosition::FixedPosition;
        cfg.fixedPos = QPoint(m_fixedXSpin->value(), m_fixedYSpin->value());
    } else if (m_multiPointRadio->isChecked()) {
        cfg.positionMode = ClickPosition::MultiPoint;
    }

    // Ограничения
    cfg.clickCount   = m_limitClicksCheck->isChecked() ? m_maxClicksSpin->value() : 0;
    cfg.timeLimitMs  = m_limitTimeCheck->isChecked() ? m_maxTimeSpin->value() * 1000 : 0;

    // Рандомизация интервала
    cfg.randomizeInterval = m_randomIntervalCheck->isChecked();
    cfg.randomIntervalMin = m_randomMinSpin->value();
    cfg.randomIntervalMax = m_randomMaxSpin->value();

    // Рандомизация позиции
    cfg.randomizePosition  = m_randomPosCheck->isChecked();
    cfg.randomOffsetPixels = m_randomOffsetSpin->value();

    m_clicker->setConfig(cfg);

    // Обновляем точки мультиточечного маршрута
    if (cfg.positionMode == ClickPosition::MultiPoint) {
        QVector<QPoint> points;
        for (int row = 0; row < m_pointsTable->rowCount(); ++row) {
            auto* xItem = m_pointsTable->item(row, 0);
            auto* yItem = m_pointsTable->item(row, 1);
            if (xItem && yItem) {
                points.append(QPoint(xItem->text().toInt(), yItem->text().toInt()));
            }
        }
        m_clicker->setClickPoints(points);
    }
}

// ===========================================
// Обновление GUI при смене состояния
// ===========================================

void MouseWidget::onClickerStarted()
{
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_statusLabel->setText(tr("Активен"));
    m_statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #a6e3a1; padding: 4px;");

    // Блокируем настройки во время работы
    m_clickGroup->setEnabled(false);
    m_posGroup->setEnabled(false);
    m_limitsGroup->setEnabled(false);
    m_randomGroup->setEnabled(false);
}

void MouseWidget::onClickerStopped()
{
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_statusLabel->setText(tr("Остановлен"));
    m_statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; padding: 4px;");

    // Разблокируем настройки
    m_clickGroup->setEnabled(true);
    m_posGroup->setEnabled(true);
    m_limitsGroup->setEnabled(true);
    m_randomGroup->setEnabled(true);
}

void MouseWidget::onClickerFinished()
{
    m_statusLabel->setText(tr("Завершён (лимит достигнут)"));
    m_statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #89b4fa; padding: 4px;");
    LOG_INFO(tr("Автоклик завершён по лимиту"));
}

void MouseWidget::onStatsUpdated(qint64 clicks, qint64 elapsedMs)
{
    m_clickCountLabel->setText(QString::number(clicks));

    // Форматируем время
    int hours   = static_cast<int>(elapsedMs / 3600000);
    int minutes = static_cast<int>((elapsedMs % 3600000) / 60000);
    int seconds = static_cast<int>((elapsedMs % 60000) / 1000);
    m_elapsedLabel->setText(QString("%1:%2:%3")
        .arg(hours,   2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

// ===========================================
// Захват позиции
// ===========================================

void MouseWidget::onPickPosition()
{
    m_pickPosButton->setText(tr("Кликните мышью..."));
    m_pickPosButton->setEnabled(false);
    m_pickingPosition = true;

    // Через 5 секунд захватываем текущую позицию
    QTimer::singleShot(5000, this, [this]() {
        if (m_pickingPosition) {
#ifdef Q_OS_WIN
            POINT pt;
            if (GetCursorPos(&pt)) {
                m_fixedXSpin->setValue(pt.x);
                m_fixedYSpin->setValue(pt.y);
                LOG_INFO(tr("Координаты захвачены: (%1, %2)").arg(pt.x).arg(pt.y));
            }
#endif
            m_pickPosButton->setText(tr("Захватить"));
            m_pickPosButton->setEnabled(true);
            m_pickingPosition = false;
        }
    });
}

// ===========================================
// Мультиточечный режим
// ===========================================

void MouseWidget::onAddPoint()
{
    // Добавляем строку с текущей позицией курсора
    QPoint pos = QCursor::pos();
    int row = m_pointsTable->rowCount();
    m_pointsTable->insertRow(row);

    auto* xItem = new QTableWidgetItem(QString::number(pos.x()));
    auto* yItem = new QTableWidgetItem(QString::number(pos.y()));
    auto* delayItem = new QTableWidgetItem("0");

    m_pointsTable->setItem(row, 0, xItem);
    m_pointsTable->setItem(row, 1, yItem);
    m_pointsTable->setItem(row, 2, delayItem);

    LOG_DEBUG(tr("Точка добавлена: #%1 (%2, %3)").arg(row + 1).arg(pos.x()).arg(pos.y()));
}

void MouseWidget::onRemovePoint()
{
    int row = m_pointsTable->currentRow();
    if (row >= 0) {
        m_pointsTable->removeRow(row);
    }
}

void MouseWidget::onClearPoints()
{
    m_pointsTable->setRowCount(0);
    m_clicker->clearClickPoints();
    LOG_DEBUG(tr("Все точки маршрута удалены"));
}

// ===========================================
// Обновление позиции курсора в GUI
// ===========================================

void MouseWidget::updatePositionLabel()
{
    QPoint pos = QCursor::pos();
    m_currentPosLabel->setText(tr("Курсор: (%1, %2)").arg(pos.x()).arg(pos.y()));
}
