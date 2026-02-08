// ===========================================
// SmartWidget.cpp — Полная реализация GUI умных режимов
// Подвкладки: Привязка к окну, Клик по цвету,
// Клик по изображению, Планировщик, Профили
// ===========================================

#include "SmartWidget.h"
#include "WindowFinder.h"
#include "ColorMatcher.h"
#include "ImageMatcher.h"
#include "Scheduler.h"

#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollArea>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QStyle>
#include <QFileInfo>

SmartWidget::SmartWidget(QWidget* parent)
    : QWidget(parent)
{
    // Создаём движки
    m_windowFinder = new WindowFinder(this);
    m_colorMatcher = new ColorMatcher(this);
    m_imageMatcher = new ImageMatcher(this);
    m_scheduler    = new Scheduler(this);

    // Привязываем WindowFinder к движкам
    m_colorMatcher->setWindowFinder(m_windowFinder);
    m_imageMatcher->setWindowFinder(m_windowFinder);

    setupUI();
    connectSignals();
}

SmartWidget::~SmartWidget() = default;

void SmartWidget::emergencyStop()
{
    if (m_colorMatcher->isRunning()) {
        m_colorMatcher->stop();
        m_colorStartBtn->setEnabled(true);
        m_colorStopBtn->setEnabled(false);
    }
    if (m_imageMatcher->isRunning()) {
        m_imageMatcher->stop();
        m_imageStartBtn->setEnabled(true);
        m_imageStopBtn->setEnabled(false);
    }
    LOG_WARNING(tr("Умные режимы: экстренная остановка"));
}

// ==========================================
// Построение интерфейса
// ==========================================

void SmartWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_subTabs = new QTabWidget(this);
    m_subTabs->setTabPosition(QTabWidget::West);

    // Подвкладка 1 — Привязка к окну
    auto* windowTab = new QWidget();
    setupWindowBindingTab(windowTab);
    m_subTabs->addTab(windowTab, tr("🪟 Окно"));

    // Подвкладка 2 — Клик по цвету
    auto* colorTab = new QWidget();
    setupColorMatchTab(colorTab);
    m_subTabs->addTab(colorTab, tr("🎨 Цвет"));

    // Подвкладка 3 — Клик по изображению
    auto* imageTab = new QWidget();
    setupImageMatchTab(imageTab);
    m_subTabs->addTab(imageTab, tr("🖼 Изображение"));

    // Подвкладка 4 — Планировщик
    auto* schedTab = new QWidget();
    setupSchedulerTab(schedTab);
    m_subTabs->addTab(schedTab, tr("⏰ Планировщик"));

    // Подвкладка 5 — Профили
    auto* profileTab = new QWidget();
    setupProfilesTab(profileTab);
    m_subTabs->addTab(profileTab, tr("📁 Профили"));

    mainLayout->addWidget(m_subTabs);
}

// ==========================================
// Подвкладка: Привязка к окну
// ==========================================

void SmartWidget::setupWindowBindingTab(QWidget* tab)
{
    auto* scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget();
    auto* layout  = new QVBoxLayout(content);

    // Группа: Выбор окна
    m_bindGroup = new QGroupBox(tr("Привязка к окну"), content);
    auto* bindLayout = new QVBoxLayout(m_bindGroup);

    // Включение
    m_bindEnable = new QCheckBox(tr("Включить привязку к окну"), m_bindGroup);
    bindLayout->addWidget(m_bindEnable);

    // Выбор окна из списка
    auto* selectLayout = new QHBoxLayout();
    m_windowCombo = new QComboBox(m_bindGroup);
    m_windowCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_windowCombo->setPlaceholderText(tr("Выберите окно..."));
    selectLayout->addWidget(m_windowCombo);

    m_refreshBtn = new QPushButton(tr("🔄"), m_bindGroup);
    m_refreshBtn->setToolTip(tr("Обновить список окон"));
    m_refreshBtn->setFixedWidth(40);
    selectLayout->addWidget(m_refreshBtn);

    m_pickWindowBtn = new QPushButton(tr("🎯 Выбрать кликом"), m_bindGroup);
    m_pickWindowBtn->setToolTip(tr("Кликните по нужному окну"));
    selectLayout->addWidget(m_pickWindowBtn);
    bindLayout->addLayout(selectLayout);

    // Информация об окне
    m_windowInfoLabel = new QLabel(tr("Окно не выбрано"), m_bindGroup);
    m_windowInfoLabel->setWordWrap(true);
    m_windowInfoLabel->setStyleSheet("color: gray; font-style: italic;");
    bindLayout->addWidget(m_windowInfoLabel);

    // Опции
    auto* optLayout = new QHBoxLayout();
    m_bringToFrontCheck = new QCheckBox(tr("Выводить окно на передний план"), m_bindGroup);
    optLayout->addWidget(m_bringToFrontCheck);

    m_useClientAreaCheck = new QCheckBox(tr("Координаты клиентской области"), m_bindGroup);
    m_useClientAreaCheck->setChecked(true);
    optLayout->addWidget(m_useClientAreaCheck);
    bindLayout->addLayout(optLayout);

    // Превью окна
    m_windowPreviewLabel = new QLabel(m_bindGroup);
    m_windowPreviewLabel->setFixedHeight(150);
    m_windowPreviewLabel->setAlignment(Qt::AlignCenter);
    m_windowPreviewLabel->setStyleSheet("background: #2a2a3a; border: 1px solid #555;");
    m_windowPreviewLabel->setText(tr("Превью окна"));
    bindLayout->addWidget(m_windowPreviewLabel);

    layout->addWidget(m_bindGroup);
    layout->addStretch();

    scroll->setWidget(content);
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);

    // Заполняем список окон
    onRefreshWindows();
}

// ==========================================
// Подвкладка: Клик по цвету
// ==========================================

void SmartWidget::setupColorMatchTab(QWidget* tab)
{
    auto* scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget();
    auto* layout  = new QVBoxLayout(content);

    m_colorGroup = new QGroupBox(tr("Клик по цвету пикселя"), content);
    auto* colorLayout = new QVBoxLayout(m_colorGroup);

    // Выбор цвета
    auto* colorSelectLayout = new QHBoxLayout();
    m_pickColorBtn = new QPushButton(tr("🎨 Выбрать цвет..."), m_colorGroup);
    colorSelectLayout->addWidget(m_pickColorBtn);

    m_colorPreview = new QLabel(m_colorGroup);
    m_colorPreview->setFixedSize(40, 25);
    m_colorPreview->setStyleSheet("background: red; border: 1px solid #555;");
    colorSelectLayout->addWidget(m_colorPreview);

    colorSelectLayout->addWidget(new QLabel("R:", m_colorGroup));
    m_colorRSpin = new QSpinBox(m_colorGroup);
    m_colorRSpin->setRange(0, 255);
    m_colorRSpin->setValue(255);
    colorSelectLayout->addWidget(m_colorRSpin);

    colorSelectLayout->addWidget(new QLabel("G:", m_colorGroup));
    m_colorGSpin = new QSpinBox(m_colorGroup);
    m_colorGSpin->setRange(0, 255);
    m_colorGSpin->setValue(0);
    colorSelectLayout->addWidget(m_colorGSpin);

    colorSelectLayout->addWidget(new QLabel("B:", m_colorGroup));
    m_colorBSpin = new QSpinBox(m_colorGroup);
    m_colorBSpin->setRange(0, 255);
    m_colorBSpin->setValue(0);
    colorSelectLayout->addWidget(m_colorBSpin);

    colorSelectLayout->addStretch();
    colorLayout->addLayout(colorSelectLayout);

    // Настройки
    auto* paramLayout = new QFormLayout();

    m_colorTolerance = new QSpinBox(m_colorGroup);
    m_colorTolerance->setRange(0, 128);
    m_colorTolerance->setValue(10);
    m_colorTolerance->setToolTip(tr("Допустимое отклонение цвета (0-128)"));
    paramLayout->addRow(tr("Допуск:"), m_colorTolerance);

    m_colorInterval = new QSpinBox(m_colorGroup);
    m_colorInterval->setRange(10, 10000);
    m_colorInterval->setValue(100);
    m_colorInterval->setSuffix(tr(" мс"));
    paramLayout->addRow(tr("Интервал сканирования:"), m_colorInterval);

    m_colorActionCombo = new QComboBox(m_colorGroup);
    m_colorActionCombo->addItem(tr("Клик"), 0);
    m_colorActionCombo->addItem(tr("Двойной клик"), 1);
    m_colorActionCombo->addItem(tr("Переместить курсор"), 2);
    m_colorActionCombo->addItem(tr("Только уведомить"), 3);
    paramLayout->addRow(tr("Действие:"), m_colorActionCombo);

    colorLayout->addLayout(paramLayout);

    // Опции
    m_colorWaitDisappear = new QCheckBox(tr("Ждать исчезновения цвета перед повтором"), m_colorGroup);
    m_colorWaitDisappear->setChecked(true);
    colorLayout->addWidget(m_colorWaitDisappear);

    // Область поиска
    auto* areaGroup = new QGroupBox(tr("Область поиска"), m_colorGroup);
    auto* areaLayout = new QVBoxLayout(areaGroup);

    m_colorFullScreen = new QCheckBox(tr("Весь экран"), areaGroup);
    m_colorFullScreen->setChecked(true);
    areaLayout->addWidget(m_colorFullScreen);

    auto* areaInputLayout = new QHBoxLayout();
    areaInputLayout->addWidget(new QLabel("X:", areaGroup));
    m_colorAreaX = new QSpinBox(areaGroup);
    m_colorAreaX->setRange(0, 9999);
    m_colorAreaX->setEnabled(false);
    areaInputLayout->addWidget(m_colorAreaX);

    areaInputLayout->addWidget(new QLabel("Y:", areaGroup));
    m_colorAreaY = new QSpinBox(areaGroup);
    m_colorAreaY->setRange(0, 9999);
    m_colorAreaY->setEnabled(false);
    areaInputLayout->addWidget(m_colorAreaY);

    areaInputLayout->addWidget(new QLabel(tr("Ш:"), areaGroup));
    m_colorAreaW = new QSpinBox(areaGroup);
    m_colorAreaW->setRange(1, 9999);
    m_colorAreaW->setValue(200);
    m_colorAreaW->setEnabled(false);
    areaInputLayout->addWidget(m_colorAreaW);

    areaInputLayout->addWidget(new QLabel(tr("В:"), areaGroup));
    m_colorAreaH = new QSpinBox(areaGroup);
    m_colorAreaH->setRange(1, 9999);
    m_colorAreaH->setValue(200);
    m_colorAreaH->setEnabled(false);
    areaInputLayout->addWidget(m_colorAreaH);
    areaLayout->addLayout(areaInputLayout);
    colorLayout->addWidget(areaGroup);

    // При переключении «Весь экран» — блокировка спинбоксов
    connect(m_colorFullScreen, &QCheckBox::toggled, this, [this](bool checked) {
        m_colorAreaX->setEnabled(!checked);
        m_colorAreaY->setEnabled(!checked);
        m_colorAreaW->setEnabled(!checked);
        m_colorAreaH->setEnabled(!checked);
    });

    // Управление
    auto* ctrlLayout = new QHBoxLayout();
    m_colorStartBtn = new QPushButton(tr("▶ Старт"), m_colorGroup);
    m_colorStartBtn->setObjectName("startButton");
    ctrlLayout->addWidget(m_colorStartBtn);

    m_colorStopBtn = new QPushButton(tr("⏹ Стоп"), m_colorGroup);
    m_colorStopBtn->setObjectName("stopButton");
    m_colorStopBtn->setEnabled(false);
    ctrlLayout->addWidget(m_colorStopBtn);
    colorLayout->addLayout(ctrlLayout);

    m_colorStatusLabel = new QLabel(tr("Готов"), m_colorGroup);
    m_colorStatusLabel->setAlignment(Qt::AlignCenter);
    colorLayout->addWidget(m_colorStatusLabel);

    layout->addWidget(m_colorGroup);
    layout->addStretch();

    scroll->setWidget(content);
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);
}

// ==========================================
// Подвкладка: Клик по изображению
// ==========================================

void SmartWidget::setupImageMatchTab(QWidget* tab)
{
    auto* scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget();
    auto* layout  = new QVBoxLayout(content);

    m_imageGroup = new QGroupBox(tr("Клик по изображению (Template Matching)"), content);
    auto* imageLayout = new QVBoxLayout(m_imageGroup);

    // Загрузка шаблона
    auto* templLayout = new QHBoxLayout();
    m_loadTemplateBtn = new QPushButton(tr("📂 Загрузить шаблон..."), m_imageGroup);
    templLayout->addWidget(m_loadTemplateBtn);

    m_templateInfoLabel = new QLabel(tr("Шаблон не загружен"), m_imageGroup);
    m_templateInfoLabel->setStyleSheet("color: gray; font-style: italic;");
    templLayout->addWidget(m_templateInfoLabel);
    templLayout->addStretch();
    imageLayout->addLayout(templLayout);

    // Превью шаблона
    m_templatePreview = new QLabel(m_imageGroup);
    m_templatePreview->setFixedHeight(120);
    m_templatePreview->setAlignment(Qt::AlignCenter);
    m_templatePreview->setStyleSheet("background: #2a2a3a; border: 1px solid #555;");
    m_templatePreview->setText(tr("Превью шаблона"));
    imageLayout->addWidget(m_templatePreview);

    // Настройки
    auto* paramLayout = new QFormLayout();

    m_imageThreshold = new QDoubleSpinBox(m_imageGroup);
    m_imageThreshold->setRange(0.1, 1.0);
    m_imageThreshold->setValue(0.85);
    m_imageThreshold->setSingleStep(0.05);
    m_imageThreshold->setDecimals(2);
    m_imageThreshold->setToolTip(tr("Порог совпадения (0.1–1.0). Чем выше — тем точнее."));
    paramLayout->addRow(tr("Порог совпадения:"), m_imageThreshold);

    m_imageInterval = new QSpinBox(m_imageGroup);
    m_imageInterval->setRange(50, 30000);
    m_imageInterval->setValue(500);
    m_imageInterval->setSuffix(tr(" мс"));
    paramLayout->addRow(tr("Интервал сканирования:"), m_imageInterval);

    m_imageActionCombo = new QComboBox(m_imageGroup);
    m_imageActionCombo->addItem(tr("Клик по центру"), 0);
    m_imageActionCombo->addItem(tr("Двойной клик"), 1);
    m_imageActionCombo->addItem(tr("Переместить курсор"), 2);
    m_imageActionCombo->addItem(tr("Только уведомить"), 3);
    paramLayout->addRow(tr("Действие:"), m_imageActionCombo);

    imageLayout->addLayout(paramLayout);

    // Опции
    m_imageWaitDisappear = new QCheckBox(tr("Ждать исчезновения перед повтором"), m_imageGroup);
    m_imageWaitDisappear->setChecked(true);
    imageLayout->addWidget(m_imageWaitDisappear);

    m_imageFullScreen = new QCheckBox(tr("Поиск по всему экрану"), m_imageGroup);
    m_imageFullScreen->setChecked(true);
    imageLayout->addWidget(m_imageFullScreen);

    m_imageGrayscale = new QCheckBox(tr("Преобразовать в оттенки серого (быстрее)"), m_imageGroup);
    m_imageGrayscale->setChecked(true);
    imageLayout->addWidget(m_imageGrayscale);

    // Управление
    auto* ctrlLayout = new QHBoxLayout();
    m_imageStartBtn = new QPushButton(tr("▶ Старт"), m_imageGroup);
    m_imageStartBtn->setObjectName("startButton");
    ctrlLayout->addWidget(m_imageStartBtn);

    m_imageStopBtn = new QPushButton(tr("⏹ Стоп"), m_imageGroup);
    m_imageStopBtn->setObjectName("stopButton");
    m_imageStopBtn->setEnabled(false);
    ctrlLayout->addWidget(m_imageStopBtn);
    imageLayout->addLayout(ctrlLayout);

    m_imageStatusLabel = new QLabel(tr("Готов"), m_imageGroup);
    m_imageStatusLabel->setAlignment(Qt::AlignCenter);
    imageLayout->addWidget(m_imageStatusLabel);

    layout->addWidget(m_imageGroup);
    layout->addStretch();

    scroll->setWidget(content);
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);
}

// ==========================================
// Подвкладка: Планировщик
// ==========================================

void SmartWidget::setupSchedulerTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);

    m_schedGroup = new QGroupBox(tr("Планировщик макросов"), tab);
    auto* schedLayout = new QVBoxLayout(m_schedGroup);

    // Таблица заданий
    m_schedTable = new QTableWidget(0, 5, m_schedGroup);
    m_schedTable->setHorizontalHeaderLabels({
        tr("Название"), tr("Макрос"), tr("Время"),
        tr("Повтор"), tr("Статус")
    });
    m_schedTable->horizontalHeader()->setStretchLastSection(true);
    m_schedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_schedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_schedTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_schedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    schedLayout->addWidget(m_schedTable);

    // Форма добавления
    auto* addGroup = new QGroupBox(tr("Новое задание"), m_schedGroup);
    auto* addLayout = new QFormLayout(addGroup);

    m_schedNameEdit = new QLineEdit(addGroup);
    m_schedNameEdit->setPlaceholderText(tr("Название задания"));
    addLayout->addRow(tr("Название:"), m_schedNameEdit);

    m_schedMacroCombo = new QComboBox(addGroup);
    m_schedMacroCombo->setPlaceholderText(tr("Выберите макрос..."));
    addLayout->addRow(tr("Макрос:"), m_schedMacroCombo);

    m_schedDateTime = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(60), addGroup);
    m_schedDateTime->setCalendarPopup(true);
    m_schedDateTime->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
    addLayout->addRow(tr("Дата и время:"), m_schedDateTime);

    auto* repeatLayout = new QHBoxLayout();
    m_schedRepeatCheck = new QCheckBox(tr("Повторять каждые"), addGroup);
    repeatLayout->addWidget(m_schedRepeatCheck);

    m_schedRepeatInterval = new QSpinBox(addGroup);
    m_schedRepeatInterval->setRange(1, 1440);
    m_schedRepeatInterval->setValue(60);
    m_schedRepeatInterval->setSuffix(tr(" мин"));
    m_schedRepeatInterval->setEnabled(false);
    repeatLayout->addWidget(m_schedRepeatInterval);
    repeatLayout->addStretch();
    addLayout->addRow(tr("Повтор:"), repeatLayout);

    connect(m_schedRepeatCheck, &QCheckBox::toggled,
            m_schedRepeatInterval, &QSpinBox::setEnabled);

    schedLayout->addWidget(addGroup);

    // Кнопки управления
    auto* btnLayout = new QHBoxLayout();
    m_schedAddBtn = new QPushButton(tr("➕ Добавить"), m_schedGroup);
    btnLayout->addWidget(m_schedAddBtn);

    m_schedRemoveBtn = new QPushButton(tr("🗑 Удалить"), m_schedGroup);
    btnLayout->addWidget(m_schedRemoveBtn);

    m_schedToggleBtn = new QPushButton(tr("▶ Запустить планировщик"), m_schedGroup);
    m_schedToggleBtn->setObjectName("startButton");
    btnLayout->addWidget(m_schedToggleBtn);
    schedLayout->addLayout(btnLayout);

    m_schedStatusLabel = new QLabel(tr("Планировщик выключен"), m_schedGroup);
    m_schedStatusLabel->setAlignment(Qt::AlignCenter);
    schedLayout->addWidget(m_schedStatusLabel);

    layout->addWidget(m_schedGroup);

    // Заполняем таблицу из сохранённых данных
    onSchedulerTaskListChanged();
}

// ==========================================
// Подвкладка: Профили
// ==========================================

void SmartWidget::setupProfilesTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);

    m_profileGroup = new QGroupBox(tr("Управление профилями"), tab);
    auto* profLayout = new QVBoxLayout(m_profileGroup);

    m_profileList = new QListWidget(m_profileGroup);
    profLayout->addWidget(m_profileList);

    auto* nameLayout = new QHBoxLayout();
    m_profileNameEdit = new QLineEdit(m_profileGroup);
    m_profileNameEdit->setPlaceholderText(tr("Имя профиля"));
    nameLayout->addWidget(m_profileNameEdit);
    profLayout->addLayout(nameLayout);

    auto* btnLayout = new QHBoxLayout();
    m_profileSaveBtn = new QPushButton(tr("💾 Сохранить"), m_profileGroup);
    btnLayout->addWidget(m_profileSaveBtn);

    m_profileLoadBtn = new QPushButton(tr("📂 Загрузить"), m_profileGroup);
    btnLayout->addWidget(m_profileLoadBtn);

    m_profileDeleteBtn = new QPushButton(tr("🗑 Удалить"), m_profileGroup);
    btnLayout->addWidget(m_profileDeleteBtn);
    profLayout->addLayout(btnLayout);

    // Описание
    auto* descLabel = new QLabel(
        tr("Профили сохраняют все настройки модулей мыши, клавиатуры "
           "и умных режимов. Используйте меню Файл → Сохранить профиль / "
           "Загрузить профиль для быстрого доступа."), m_profileGroup);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: gray;");
    profLayout->addWidget(descLabel);

    layout->addWidget(m_profileGroup);
    layout->addStretch();
}

// ==========================================
// Подключение сигналов
// ==========================================

void SmartWidget::connectSignals()
{
    // === Привязка к окну ===
    connect(m_pickWindowBtn, &QPushButton::clicked, this, &SmartWidget::onPickWindow);
    connect(m_refreshBtn,    &QPushButton::clicked, this, &SmartWidget::onRefreshWindows);
    connect(m_windowCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SmartWidget::onWindowSelected);
    connect(m_bindEnable,    &QCheckBox::toggled, this, &SmartWidget::onBindingToggled);
    connect(m_windowFinder,  &WindowFinder::windowPicked, this, &SmartWidget::onWindowPicked);

    // === Клик по цвету ===
    connect(m_pickColorBtn,  &QPushButton::clicked, this, &SmartWidget::onPickColor);
    connect(m_colorStartBtn, &QPushButton::clicked, this, &SmartWidget::onColorStart);
    connect(m_colorStopBtn,  &QPushButton::clicked, this, &SmartWidget::onColorStop);
    connect(m_colorMatcher,  &ColorMatcher::colorFound, this, &SmartWidget::onColorFound);

    // Обновление превью цвета при изменении спинбоксов
    auto updateColorPreview = [this]() {
        m_selectedColor = QColor(m_colorRSpin->value(),
                                  m_colorGSpin->value(),
                                  m_colorBSpin->value());
        m_colorPreview->setStyleSheet(
            QString("background: %1; border: 1px solid #555;")
                .arg(m_selectedColor.name()));
    };
    connect(m_colorRSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateColorPreview);
    connect(m_colorGSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateColorPreview);
    connect(m_colorBSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateColorPreview);

    // Статус сканирования цвета
    connect(m_colorMatcher, &ColorMatcher::scanCompleted, this, [this](int count) {
        m_colorStatusLabel->setText(tr("Сканирований: %1").arg(count));
    });

    // === Клик по изображению ===
    connect(m_loadTemplateBtn, &QPushButton::clicked, this, &SmartWidget::onLoadTemplate);
    connect(m_imageStartBtn,   &QPushButton::clicked, this, &SmartWidget::onImageStart);
    connect(m_imageStopBtn,    &QPushButton::clicked, this, &SmartWidget::onImageStop);
    connect(m_imageMatcher,    &ImageMatcher::imageFound, this, &SmartWidget::onImageFound);

    connect(m_imageMatcher, &ImageMatcher::scanCompleted, this, [this](int count) {
        m_imageStatusLabel->setText(tr("Сканирований: %1").arg(count));
    });

    // === Планировщик ===
    connect(m_schedAddBtn,    &QPushButton::clicked, this, &SmartWidget::onAddSchedulerTask);
    connect(m_schedRemoveBtn, &QPushButton::clicked, this, &SmartWidget::onRemoveSchedulerTask);
    connect(m_schedToggleBtn, &QPushButton::clicked, this, &SmartWidget::onSchedulerToggle);
    connect(m_scheduler, &Scheduler::taskTriggered, this, &SmartWidget::onTaskTriggered);
    connect(m_scheduler, &Scheduler::taskListChanged, this, &SmartWidget::onSchedulerTaskListChanged);

    connect(m_scheduler, &Scheduler::nextTriggerInfo, this,
        [this](const QString& taskName, const QDateTime& time) {
            qint64 secsLeft = QDateTime::currentDateTime().secsTo(time);
            QString timeStr;
            if (secsLeft < 60) {
                timeStr = tr("%1 сек").arg(secsLeft);
            } else if (secsLeft < 3600) {
                timeStr = tr("%1 мин").arg(secsLeft / 60);
            } else {
                timeStr = tr("%1 ч %2 мин").arg(secsLeft / 3600).arg((secsLeft % 3600) / 60);
            }
            m_schedStatusLabel->setText(
                tr("Планировщик работает | Следующее: \"%1\" через %2")
                    .arg(taskName, timeStr));
        });

    // === Профили ===
    // Профили управляются через меню Файл → Сохранить/Загрузить профиль
    // Здесь дублируем для удобства
    connect(m_profileSaveBtn,   &QPushButton::clicked, this, [this]() {
        QString name = m_profileNameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Введите имя профиля"));
            return;
        }
        LOG_INFO(tr("Запрос на сохранение профиля: %1 (используйте меню Файл)").arg(name));
    });

    connect(m_profileLoadBtn,   &QPushButton::clicked, this, [this]() {
        auto* item = m_profileList->currentItem();
        if (!item) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Выберите профиль"));
            return;
        }
        LOG_INFO(tr("Запрос на загрузку профиля: %1 (используйте меню Файл)").arg(item->text()));
    });

    connect(m_profileDeleteBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_profileList->currentItem();
        if (!item) return;
        LOG_INFO(tr("Запрос на удаление профиля: %1").arg(item->text()));
    });
}

// ==========================================
// Слоты: Привязка к окну
// ==========================================

void SmartWidget::onPickWindow()
{
    m_pickWindowBtn->setText(tr("🎯 Кликните по окну..."));
    m_pickWindowBtn->setEnabled(false);
    m_windowFinder->startPickWindow();
}

void SmartWidget::onWindowPicked(quintptr hwnd, const QString& title)
{
    m_pickWindowBtn->setText(tr("🎯 Выбрать кликом"));
    m_pickWindowBtn->setEnabled(true);

    // Обновляем информацию
    WindowInfo info = m_windowFinder->targetWindowInfo();
    m_windowInfoLabel->setText(
        tr("Окно: %1\nКласс: %2\nРазмер: %3x%4\nPID: %5")
            .arg(info.title, info.className)
            .arg(info.geometry.width()).arg(info.geometry.height())
            .arg(info.processId));
    m_windowInfoLabel->setStyleSheet("color: #89b4fa;");

    // Обновляем превью
    QPixmap preview = m_windowFinder->captureWindow();
    if (!preview.isNull()) {
        m_windowPreviewLabel->setPixmap(
            preview.scaled(m_windowPreviewLabel->size(),
                           Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // Выбираем в комбобоксе
    for (int i = 0; i < m_windowHandles.size(); ++i) {
        if (m_windowHandles[i] == hwnd) {
            m_windowCombo->setCurrentIndex(i);
            break;
        }
    }

    LOG_INFO(tr("Привязка к окну: %1").arg(title));
}

void SmartWidget::onRefreshWindows()
{
    m_windowCombo->clear();
    m_windowHandles.clear();

    auto windows = m_windowFinder->enumerateWindows();
    for (const auto& win : windows) {
        QString text = QString("%1 [%2]").arg(win.title, win.className);
        m_windowCombo->addItem(text);
        m_windowHandles.append(win.handle);
    }
}

void SmartWidget::onWindowSelected(int index)
{
    if (index < 0 || index >= m_windowHandles.size()) return;

    quintptr hwnd = m_windowHandles[index];
    m_windowFinder->setTargetWindow(hwnd);

    WindowInfo info = m_windowFinder->targetWindowInfo();
    m_windowInfoLabel->setText(
        tr("Окно: %1\nКласс: %2\nРазмер: %3x%4\nPID: %5")
            .arg(info.title, info.className)
            .arg(info.geometry.width()).arg(info.geometry.height())
            .arg(info.processId));
    m_windowInfoLabel->setStyleSheet("color: #89b4fa;");

    // Обновляем превью
    QPixmap preview = m_windowFinder->captureWindow();
    if (!preview.isNull()) {
        m_windowPreviewLabel->setPixmap(
            preview.scaled(m_windowPreviewLabel->size(),
                           Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void SmartWidget::onBindingToggled(bool enabled)
{
    if (enabled) {
        if (m_windowFinder->targetHandle() == 0) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Сначала выберите окно"));
            m_bindEnable->setChecked(false);
            return;
        }
        LOG_INFO(tr("Привязка к окну включена"));
    } else {
        LOG_INFO(tr("Привязка к окну выключена"));
    }
}

// ==========================================
// Слоты: Клик по цвету
// ==========================================

void SmartWidget::onPickColor()
{
    QColor color = QColorDialog::getColor(m_selectedColor, this,
        tr("Выберите целевой цвет"));
    if (color.isValid()) {
        m_selectedColor = color;
        m_colorRSpin->setValue(color.red());
        m_colorGSpin->setValue(color.green());
        m_colorBSpin->setValue(color.blue());
        m_colorPreview->setStyleSheet(
            QString("background: %1; border: 1px solid #555;").arg(color.name()));
    }
}

void SmartWidget::onColorStart()
{
    // Собираем конфигурацию
    ColorMatchConfig cfg;
    cfg.enabled          = true;
    cfg.targetColor      = m_selectedColor;
    cfg.tolerance        = m_colorTolerance->value();
    cfg.scanIntervalMs   = m_colorInterval->value();
    cfg.action           = static_cast<ColorMatchConfig::Action>(
                               m_colorActionCombo->currentIndex());
    cfg.waitForDisappear = m_colorWaitDisappear->isChecked();
    cfg.searchFullScreen = m_colorFullScreen->isChecked();
    cfg.searchArea       = QRect(m_colorAreaX->value(), m_colorAreaY->value(),
                                  m_colorAreaW->value(), m_colorAreaH->value());

    m_colorMatcher->setConfig(cfg);
    m_colorMatcher->start();

    m_colorStartBtn->setEnabled(false);
    m_colorStopBtn->setEnabled(true);
    m_colorStatusLabel->setText(tr("Сканирование..."));
}

void SmartWidget::onColorStop()
{
    m_colorMatcher->stop();
    m_colorStartBtn->setEnabled(true);
    m_colorStopBtn->setEnabled(false);
    m_colorStatusLabel->setText(tr("Остановлен"));
}

void SmartWidget::onColorFound(const QPoint& pos)
{
    m_colorStatusLabel->setText(
        tr("Цвет найден: (%1, %2)").arg(pos.x()).arg(pos.y()));
}

// ==========================================
// Слоты: Клик по изображению
// ==========================================

void SmartWidget::onLoadTemplate()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Выберите изображение-шаблон"),
        QString(),
        tr("Изображения (*.png *.jpg *.bmp *.gif);;Все файлы (*)"));

    if (path.isEmpty()) return;

    if (m_imageMatcher->loadTemplate(path)) {
        // Показываем превью
        QPixmap preview(path);
        if (!preview.isNull()) {
            m_templatePreview->setPixmap(
                preview.scaled(m_templatePreview->size(),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_templateInfoLabel->setText(
                tr("%1 (%2x%3)")
                    .arg(QFileInfo(path).fileName())
                    .arg(preview.width()).arg(preview.height()));
            m_templateInfoLabel->setStyleSheet("color: #a6e3a1;");
        }
    }
}

void SmartWidget::onImageStart()
{
    // Собираем конфигурацию
    ImageMatchConfig cfg;
    cfg.enabled          = true;
    cfg.threshold        = m_imageThreshold->value();
    cfg.scanIntervalMs   = m_imageInterval->value();
    cfg.action           = static_cast<ImageMatchConfig::Action>(
                               m_imageActionCombo->currentIndex());
    cfg.waitForDisappear = m_imageWaitDisappear->isChecked();
    cfg.searchFullScreen = m_imageFullScreen->isChecked();
    cfg.useGrayscale     = m_imageGrayscale->isChecked();

    m_imageMatcher->setConfig(cfg);
    m_imageMatcher->start();

    m_imageStartBtn->setEnabled(false);
    m_imageStopBtn->setEnabled(true);
    m_imageStatusLabel->setText(tr("Сканирование..."));
}

void SmartWidget::onImageStop()
{
    m_imageMatcher->stop();
    m_imageStartBtn->setEnabled(true);
    m_imageStopBtn->setEnabled(false);
    m_imageStatusLabel->setText(tr("Остановлен"));
}

void SmartWidget::onImageFound(const QPoint& pos, double score)
{
    m_imageStatusLabel->setText(
        tr("Изображение найдено: (%1, %2), совпадение: %3%")
            .arg(pos.x()).arg(pos.y()).arg(static_cast<int>(score * 100)));
}

// ==========================================
// Слоты: Планировщик
// ==========================================

void SmartWidget::onAddSchedulerTask()
{
    QString name = m_schedNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Введите название задания"));
        return;
    }

    if (m_schedMacroCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Выберите макрос"));
        return;
    }

    SchedulerTask task;
    task.id                = Scheduler::generateTaskId();
    task.name              = name;
    task.macroName         = m_schedMacroCombo->currentText();
    task.scheduledTime     = m_schedDateTime->dateTime();
    task.repeat            = m_schedRepeatCheck->isChecked();
    task.repeatIntervalMin = m_schedRepeatInterval->value();
    task.enabled           = true;

    m_scheduler->addTask(task);
    m_schedNameEdit->clear();
}

void SmartWidget::onRemoveSchedulerTask()
{
    int row = m_schedTable->currentRow();
    if (row < 0) return;

    auto tasks = m_scheduler->allTasks();
    if (row < tasks.size()) {
        m_scheduler->removeTask(tasks[row].id);
    }
}

void SmartWidget::onSchedulerToggle()
{
    if (m_scheduler->isRunning()) {
        m_scheduler->stop();
        m_schedToggleBtn->setText(tr("▶ Запустить планировщик"));
        m_schedToggleBtn->setObjectName("startButton");
        m_schedStatusLabel->setText(tr("Планировщик выключен"));
    } else {
        m_scheduler->start();
        m_schedToggleBtn->setText(tr("⏹ Остановить планировщик"));
        m_schedToggleBtn->setObjectName("stopButton");
        m_schedStatusLabel->setText(tr("Планировщик работает..."));
    }

    // Обновляем стиль кнопки
    m_schedToggleBtn->style()->unpolish(m_schedToggleBtn);
    m_schedToggleBtn->style()->polish(m_schedToggleBtn);
}

void SmartWidget::onTaskTriggered(const QString& macroName)
{
    emit runMacroRequested(macroName);
}

void SmartWidget::onSchedulerTaskListChanged()
{
    auto tasks = m_scheduler->allTasks();
    m_schedTable->setRowCount(tasks.size());

    for (int i = 0; i < tasks.size(); ++i) {
        const auto& task = tasks[i];

        m_schedTable->setItem(i, 0, new QTableWidgetItem(task.name));
        m_schedTable->setItem(i, 1, new QTableWidgetItem(task.macroName));
        m_schedTable->setItem(i, 2, new QTableWidgetItem(
            task.scheduledTime.toString("dd.MM.yyyy HH:mm:ss")));

        QString repeatStr = task.repeat ?
            tr("Каждые %1 мин").arg(task.repeatIntervalMin) : tr("Однократно");
        m_schedTable->setItem(i, 3, new QTableWidgetItem(repeatStr));

        QString statusStr = task.enabled ? tr("✅ Активно") : tr("⏸ Выключено");
        m_schedTable->setItem(i, 4, new QTableWidgetItem(statusStr));
    }
}

// ==========================================
// Список макросов (устанавливается из MainWindow)
// ==========================================

void SmartWidget::setAvailableMacros(const QStringList& macros)
{
    m_schedMacroCombo->clear();
    m_schedMacroCombo->addItems(macros);
}

// ==========================================
// Профили: сбор и применение настроек
// ==========================================

QJsonObject SmartWidget::collectSettings() const
{
    QJsonObject obj;

    // Привязка к окну
    WindowBindingConfig wbc;
    wbc.enabled = m_bindEnable->isChecked();
    if (m_windowFinder->targetHandle() != 0) {
        WindowInfo info = m_windowFinder->targetWindowInfo();
        wbc.windowTitle  = info.title;
        wbc.windowClass  = info.className;
    }
    wbc.bringToFront  = m_bringToFrontCheck->isChecked();
    wbc.useClientArea = m_useClientAreaCheck->isChecked();
    obj["windowBinding"] = wbc.toJson();

    // Настройки клика по цвету
    ColorMatchConfig cmc;
    cmc.targetColor      = m_selectedColor;
    cmc.tolerance        = m_colorTolerance->value();
    cmc.scanIntervalMs   = m_colorInterval->value();
    cmc.action           = static_cast<ColorMatchConfig::Action>(
                               m_colorActionCombo->currentIndex());
    cmc.waitForDisappear = m_colorWaitDisappear->isChecked();
    cmc.searchFullScreen = m_colorFullScreen->isChecked();
    cmc.searchArea       = QRect(m_colorAreaX->value(), m_colorAreaY->value(),
                                  m_colorAreaW->value(), m_colorAreaH->value());
    obj["colorMatch"] = cmc.toJson();

    // Настройки клика по изображению
    ImageMatchConfig imc;
    imc.templatePath     = m_imageMatcher->config().templatePath;
    imc.threshold        = m_imageThreshold->value();
    imc.scanIntervalMs   = m_imageInterval->value();
    imc.action           = static_cast<ImageMatchConfig::Action>(
                               m_imageActionCombo->currentIndex());
    imc.waitForDisappear = m_imageWaitDisappear->isChecked();
    imc.searchFullScreen = m_imageFullScreen->isChecked();
    imc.useGrayscale     = m_imageGrayscale->isChecked();
    obj["imageMatch"] = imc.toJson();

    // Задания планировщика
    obj["scheduler"] = m_scheduler->toJson();

    return obj;
}

void SmartWidget::applySettings(const QJsonObject& settings)
{
    // Привязка к окну
    if (settings.contains("windowBinding")) {
        auto wbc = WindowBindingConfig::fromJson(settings["windowBinding"].toObject());
        m_bindEnable->setChecked(wbc.enabled);
        m_bringToFrontCheck->setChecked(wbc.bringToFront);
        m_useClientAreaCheck->setChecked(wbc.useClientArea);
        if (!wbc.windowTitle.isEmpty()) {
            m_windowFinder->setTargetByTitle(wbc.windowTitle);
        }
    }

    // Клик по цвету
    if (settings.contains("colorMatch")) {
        auto cmc = ColorMatchConfig::fromJson(settings["colorMatch"].toObject());
        m_selectedColor = cmc.targetColor;
        m_colorRSpin->setValue(cmc.targetColor.red());
        m_colorGSpin->setValue(cmc.targetColor.green());
        m_colorBSpin->setValue(cmc.targetColor.blue());
        m_colorPreview->setStyleSheet(
            QString("background: %1; border: 1px solid #555;").arg(cmc.targetColor.name()));
        m_colorTolerance->setValue(cmc.tolerance);
        m_colorInterval->setValue(cmc.scanIntervalMs);
        m_colorActionCombo->setCurrentIndex(static_cast<int>(cmc.action));
        m_colorWaitDisappear->setChecked(cmc.waitForDisappear);
        m_colorFullScreen->setChecked(cmc.searchFullScreen);
        m_colorAreaX->setValue(cmc.searchArea.x());
        m_colorAreaY->setValue(cmc.searchArea.y());
        m_colorAreaW->setValue(cmc.searchArea.width());
        m_colorAreaH->setValue(cmc.searchArea.height());
    }

    // Клик по изображению
    if (settings.contains("imageMatch")) {
        auto imc = ImageMatchConfig::fromJson(settings["imageMatch"].toObject());
        m_imageThreshold->setValue(imc.threshold);
        m_imageInterval->setValue(imc.scanIntervalMs);
        m_imageActionCombo->setCurrentIndex(static_cast<int>(imc.action));
        m_imageWaitDisappear->setChecked(imc.waitForDisappear);
        m_imageFullScreen->setChecked(imc.searchFullScreen);
        m_imageGrayscale->setChecked(imc.useGrayscale);
        if (!imc.templatePath.isEmpty()) {
            m_imageMatcher->loadTemplate(imc.templatePath);
        }
    }

    // Планировщик
    if (settings.contains("scheduler")) {
        m_scheduler->fromJson(settings["scheduler"].toArray());
        onSchedulerTaskListChanged();
    }
}
