// ===========================================
// MacroEditor.cpp — Визуальный редактор макросов
// Drag & drop, добавление/редактирование действий
// ===========================================

#include "MacroEditor.h"
#include "keyboard/KeyboardClicker.h"
#include "utils/Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QScrollArea>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

MacroEditor::MacroEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    installEventFilter(this);
}

// ===========================================
// Построение интерфейса
// ===========================================

void MacroEditor::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ==========================================
    // Левая часть: тулбар + список действий
    // ==========================================
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    setupToolbar();
    leftLayout->addLayout([this]() {
        auto* toolbar = new QHBoxLayout();
        toolbar->addWidget(m_btnAdd);
        toolbar->addWidget(m_btnRemove);
        toolbar->addWidget(m_btnMoveUp);
        toolbar->addWidget(m_btnMoveDown);
        toolbar->addWidget(m_btnDuplicate);
        toolbar->addStretch();
        return toolbar;
    }());

    setupActionList();
    leftLayout->addWidget(m_actionList);

    // ==========================================
    // Правая часть: панель параметров
    // ==========================================
    setupParamsPanel();

    // Добавляем в основной layout: 60% список, 40% параметры
    mainLayout->addWidget(leftWidget, 6);
    mainLayout->addWidget(m_paramsGroup, 4);
}

void MacroEditor::setupActionList()
{
    m_actionList = new QListWidget(this);
    m_actionList->setAlternatingRowColors(true);
    m_actionList->setSelectionMode(QAbstractItemView::SingleSelection);

    // Drag & Drop для перестановки
    m_actionList->setDragDropMode(QAbstractItemView::InternalMove);
    m_actionList->setDefaultDropAction(Qt::MoveAction);
    m_actionList->setDragEnabled(true);

    connect(m_actionList, &QListWidget::itemSelectionChanged,
            this, &MacroEditor::onItemSelectionChanged);

    // Обработка перетаскивания — синхронизируем m_actions с визуальным порядком
    connect(m_actionList->model(), &QAbstractItemModel::rowsMoved,
            this, [this](const QModelIndex&, int /*start*/, int /*end*/,
                          const QModelIndex&, int /*dest*/) {
        // Пересобираем m_actions из списка
        QList<MacroAction> reordered;
        for (int i = 0; i < m_actionList->count(); ++i) {
            int origIdx = m_actionList->item(i)->data(Qt::UserRole).toInt();
            if (origIdx >= 0 && origIdx < m_actions.size()) {
                reordered.append(m_actions[origIdx]);
            }
        }
        if (reordered.size() == m_actions.size()) {
            m_actions = reordered;
            // Обновляем индексы
            for (int i = 0; i < m_actionList->count(); ++i) {
                m_actionList->item(i)->setData(Qt::UserRole, i);
            }
            emit actionsChanged();
        }
    });
}

void MacroEditor::setupToolbar()
{
    m_btnAdd      = new QPushButton(tr("➕ Добавить"), this);
    m_btnRemove   = new QPushButton(tr("🗑 Удалить"), this);
    m_btnMoveUp   = new QPushButton(tr("⬆"), this);
    m_btnMoveDown = new QPushButton(tr("⬇"), this);
    m_btnDuplicate = new QPushButton(tr("📋"), this);

    m_btnMoveUp->setFixedWidth(32);
    m_btnMoveDown->setFixedWidth(32);
    m_btnDuplicate->setFixedWidth(32);

    m_btnMoveUp->setToolTip(tr("Переместить вверх"));
    m_btnMoveDown->setToolTip(tr("Переместить вниз"));
    m_btnDuplicate->setToolTip(tr("Дублировать"));

    createAddMenu();
    m_btnAdd->setMenu(m_addMenu);

    connect(m_btnRemove,   &QPushButton::clicked, this, &MacroEditor::onRemoveSelected);
    connect(m_btnMoveUp,   &QPushButton::clicked, this, &MacroEditor::onMoveUp);
    connect(m_btnMoveDown, &QPushButton::clicked, this, &MacroEditor::onMoveDown);
    connect(m_btnDuplicate,&QPushButton::clicked, this, &MacroEditor::onDuplicateSelected);
}

void MacroEditor::createAddMenu()
{
    m_addMenu = new QMenu(this);

    // Мышь
    auto* mouseMenu = m_addMenu->addMenu(tr("🖱 Мышь"));
    mouseMenu->addAction(tr("Клик мышью"), this, [this]() {
        addActionOfType(MacroActionType::MouseClick);
    });
    mouseMenu->addAction(tr("Двойной клик"), this, [this]() {
        addActionOfType(MacroActionType::MouseDoubleClick);
    });
    mouseMenu->addAction(tr("Нажать кнопку"), this, [this]() {
        addActionOfType(MacroActionType::MouseDown);
    });
    mouseMenu->addAction(tr("Отпустить кнопку"), this, [this]() {
        addActionOfType(MacroActionType::MouseUp);
    });
    mouseMenu->addAction(tr("Переместить курсор"), this, [this]() {
        addActionOfType(MacroActionType::MouseMove);
    });

    // Клавиатура
    auto* kbMenu = m_addMenu->addMenu(tr("⌨ Клавиатура"));
    kbMenu->addAction(tr("Нажать клавишу"), this, [this]() {
        addActionOfType(MacroActionType::KeyPress);
    });
    kbMenu->addAction(tr("Нажать (без отпускания)"), this, [this]() {
        addActionOfType(MacroActionType::KeyDown);
    });
    kbMenu->addAction(tr("Отпустить клавишу"), this, [this]() {
        addActionOfType(MacroActionType::KeyUp);
    });
    kbMenu->addAction(tr("Комбинация клавиш"), this, [this]() {
        addActionOfType(MacroActionType::KeyCombo);
    });
    kbMenu->addAction(tr("Ввести текст"), this, [this]() {
        addActionOfType(MacroActionType::TypeText);
    });

    // Управление
    m_addMenu->addSeparator();
    m_addMenu->addAction(tr("⏱ Задержка"), this, [this]() {
        addActionOfType(MacroActionType::Delay);
    });
    m_addMenu->addAction(tr("🎲 Случайная задержка"), this, [this]() {
        addActionOfType(MacroActionType::RandomDelay);
    });

    // Циклы
    m_addMenu->addSeparator();
    m_addMenu->addAction(tr("🔁 Начало цикла"), this, [this]() {
        addActionOfType(MacroActionType::LoopStart);
    });
    m_addMenu->addAction(tr("🔁 Конец цикла"), this, [this]() {
        addActionOfType(MacroActionType::LoopEnd);
    });

    // Вложенные макросы и комментарии
    m_addMenu->addSeparator();
    m_addMenu->addAction(tr("📦 Вложенный макрос"), this, [this]() {
        addActionOfType(MacroActionType::SubMacro);
    });
    m_addMenu->addAction(tr("💬 Комментарий"), this, [this]() {
        addActionOfType(MacroActionType::Comment);
    });
}

void MacroEditor::setupParamsPanel()
{
    m_paramsGroup = new QGroupBox(tr("Параметры действия"), this);
    auto* paramsLayout = new QVBoxLayout(m_paramsGroup);

    m_paramsStack = new QStackedWidget(m_paramsGroup);

    // ==========================================
    // Страница 0: Пустая (ничего не выбрано)
    // ==========================================
    auto* emptyPage = new QWidget();
    auto* emptyLayout = new QVBoxLayout(emptyPage);
    auto* emptyLabel = new QLabel(tr("Выберите действие\nдля редактирования"), emptyPage);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    m_paramsStack->addWidget(emptyPage);  // index 0

    // ==========================================
    // Страница 1: Мышь (клик, двойной, down, up)
    // ==========================================
    auto* mousePage = new QWidget();
    auto* mouseLayout = new QFormLayout(mousePage);

    m_mouseButtonCombo = new QComboBox(mousePage);
    m_mouseButtonCombo->addItem(tr("Левая (ЛКМ)"), 0);
    m_mouseButtonCombo->addItem(tr("Правая (ПКМ)"), 1);
    m_mouseButtonCombo->addItem(tr("Средняя (СКМ)"), 2);
    mouseLayout->addRow(tr("Кнопка:"), m_mouseButtonCombo);

    m_useCurrentPosCheck = new QCheckBox(tr("Текущая позиция курсора"), mousePage);
    mouseLayout->addRow(m_useCurrentPosCheck);

    m_posXSpin = new QSpinBox(mousePage);
    m_posXSpin->setRange(0, 9999);
    mouseLayout->addRow(tr("X:"), m_posXSpin);

    m_posYSpin = new QSpinBox(mousePage);
    m_posYSpin->setRange(0, 9999);
    mouseLayout->addRow(tr("Y:"), m_posYSpin);

    connect(m_useCurrentPosCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_posXSpin->setEnabled(!checked);
        m_posYSpin->setEnabled(!checked);
    });

    // Кнопка «Применить» для мыши
    auto* mouseApplyBtn = new QPushButton(tr("Применить"), mousePage);
    connect(mouseApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    mouseLayout->addRow(mouseApplyBtn);

    m_paramsStack->addWidget(mousePage);  // index 1

    // ==========================================
    // Страница 2: Клавиатура (KeyDown/KeyUp/KeyPress/KeyCombo)
    // ==========================================
    auto* kbPage = new QWidget();
    auto* kbLayout = new QFormLayout(kbPage);

    m_vkCodeSpin = new QSpinBox(kbPage);
    m_vkCodeSpin->setRange(0, 255);
    kbLayout->addRow(tr("VK-код:"), m_vkCodeSpin);

    m_keyNameLabel = new QLabel(tr("—"), kbPage);
    kbLayout->addRow(tr("Клавиша:"), m_keyNameLabel);

    m_captureKeyBtn = new QPushButton(tr("🎯 Захватить клавишу"), kbPage);
    kbLayout->addRow(m_captureKeyBtn);

    m_ctrlCheck  = new QCheckBox(tr("Ctrl"), kbPage);
    m_shiftCheck = new QCheckBox(tr("Shift"), kbPage);
    m_altCheck   = new QCheckBox(tr("Alt"), kbPage);
    m_winCheck   = new QCheckBox(tr("Win"), kbPage);

    auto* modLayout = new QHBoxLayout();
    modLayout->addWidget(m_ctrlCheck);
    modLayout->addWidget(m_shiftCheck);
    modLayout->addWidget(m_altCheck);
    modLayout->addWidget(m_winCheck);
    kbLayout->addRow(tr("Модификаторы:"), modLayout);

    connect(m_vkCodeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        m_keyNameLabel->setText(KeyboardClicker::vkCodeToName(val));
    });

    connect(m_captureKeyBtn, &QPushButton::clicked, this, [this]() {
        m_capturing = true;
        m_captureKeyBtn->setText(tr("⏳ Нажмите клавишу..."));
        m_captureKeyBtn->setFocus();
    });

    auto* kbApplyBtn = new QPushButton(tr("Применить"), kbPage);
    connect(kbApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    kbLayout->addRow(kbApplyBtn);

    m_paramsStack->addWidget(kbPage);  // index 2

    // ==========================================
    // Страница 3: TypeText
    // ==========================================
    auto* textPage = new QWidget();
    auto* textLayout = new QFormLayout(textPage);

    m_textEdit = new QTextEdit(textPage);
    m_textEdit->setMaximumHeight(100);
    textLayout->addRow(tr("Текст:"), m_textEdit);

    m_typeDelaySpin = new QSpinBox(textPage);
    m_typeDelaySpin->setRange(0, 5000);
    m_typeDelaySpin->setSuffix(tr(" мс"));
    m_typeDelaySpin->setValue(30);
    textLayout->addRow(tr("Задержка между символами:"), m_typeDelaySpin);

    auto* textApplyBtn = new QPushButton(tr("Применить"), textPage);
    connect(textApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    textLayout->addRow(textApplyBtn);

    m_paramsStack->addWidget(textPage);  // index 3

    // ==========================================
    // Страница 4: Delay
    // ==========================================
    auto* delayPage = new QWidget();
    auto* delayLayout = new QFormLayout(delayPage);

    m_delaySpin = new QSpinBox(delayPage);
    m_delaySpin->setRange(0, 3600000);
    m_delaySpin->setSuffix(tr(" мс"));
    m_delaySpin->setValue(100);
    delayLayout->addRow(tr("Задержка:"), m_delaySpin);

    auto* delayApplyBtn = new QPushButton(tr("Применить"), delayPage);
    connect(delayApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    delayLayout->addRow(delayApplyBtn);

    m_paramsStack->addWidget(delayPage); // index 4

    // ==========================================
    // Страница 5: RandomDelay
    // ==========================================
    auto* rndDelayPage = new QWidget();
    auto* rndDelayLayout = new QFormLayout(rndDelayPage);

    m_delayMinSpin = new QSpinBox(rndDelayPage);
    m_delayMinSpin->setRange(0, 3600000);
    m_delayMinSpin->setSuffix(tr(" мс"));
    m_delayMinSpin->setValue(50);
    rndDelayLayout->addRow(tr("Мин. задержка:"), m_delayMinSpin);

    m_delayMaxSpin = new QSpinBox(rndDelayPage);
    m_delayMaxSpin->setRange(0, 3600000);
    m_delayMaxSpin->setSuffix(tr(" мс"));
    m_delayMaxSpin->setValue(200);
    rndDelayLayout->addRow(tr("Макс. задержка:"), m_delayMaxSpin);

    auto* rndDelayApplyBtn = new QPushButton(tr("Применить"), rndDelayPage);
    connect(rndDelayApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    rndDelayLayout->addRow(rndDelayApplyBtn);

    m_paramsStack->addWidget(rndDelayPage); // index 5

    // ==========================================
    // Страница 6: Loop
    // ==========================================
    auto* loopPage = new QWidget();
    auto* loopLayout = new QFormLayout(loopPage);

    m_loopCountSpin = new QSpinBox(loopPage);
    m_loopCountSpin->setRange(0, AppConstants::Macro::MAX_LOOP_COUNT);
    m_loopCountSpin->setSpecialValueText(tr("∞ (бесконечно)"));
    m_loopCountSpin->setValue(1);
    loopLayout->addRow(tr("Повторений:"), m_loopCountSpin);

    auto* loopApplyBtn = new QPushButton(tr("Применить"), loopPage);
    connect(loopApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    loopLayout->addRow(loopApplyBtn);

    m_paramsStack->addWidget(loopPage); // index 6

    // ==========================================
    // Страница 7: SubMacro
    // ==========================================
    auto* subPage = new QWidget();
    auto* subLayout = new QFormLayout(subPage);

    m_subMacroEdit = new QLineEdit(subPage);
    m_subMacroEdit->setPlaceholderText(tr("Имя вложенного макроса"));
    subLayout->addRow(tr("Макрос:"), m_subMacroEdit);

    auto* subApplyBtn = new QPushButton(tr("Применить"), subPage);
    connect(subApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    subLayout->addRow(subApplyBtn);

    m_paramsStack->addWidget(subPage); // index 7

    // ==========================================
    // Страница 8: Comment
    // ==========================================
    auto* commentPage = new QWidget();
    auto* commentLayout = new QFormLayout(commentPage);

    m_commentEdit = new QLineEdit(commentPage);
    m_commentEdit->setPlaceholderText(tr("Текст комментария"));
    commentLayout->addRow(tr("Комментарий:"), m_commentEdit);

    auto* commentApplyBtn = new QPushButton(tr("Применить"), commentPage);
    connect(commentApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    commentLayout->addRow(commentApplyBtn);

    m_paramsStack->addWidget(commentPage); // index 8

    // ==========================================
    // Страница 9: MouseMove
    // ==========================================
    auto* movePage = new QWidget();
    auto* moveLayout = new QFormLayout(movePage);

    m_movePosXSpin = new QSpinBox(movePage);
    m_movePosXSpin->setRange(0, 9999);
    moveLayout->addRow(tr("X:"), m_movePosXSpin);

    m_movePosYSpin = new QSpinBox(movePage);
    m_movePosYSpin->setRange(0, 9999);
    moveLayout->addRow(tr("Y:"), m_movePosYSpin);

    auto* moveApplyBtn = new QPushButton(tr("Применить"), movePage);
    connect(moveApplyBtn, &QPushButton::clicked, this, &MacroEditor::onEditApplied);
    moveLayout->addRow(moveApplyBtn);

    m_paramsStack->addWidget(movePage); // index 9

    // ==========================================
    paramsLayout->addWidget(m_paramsStack);
    paramsLayout->addStretch();
}

// ===========================================
// Работа с действиями
// ===========================================

void MacroEditor::setActions(const QList<MacroAction>& actions)
{
    m_actions = actions;
    m_actionList->clear();

    for (int i = 0; i < m_actions.size(); ++i) {
        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, i);
        m_actionList->addItem(item);
        updateListItem(i);
    }

    m_paramsStack->setCurrentIndex(PageEmpty);
    emit actionsChanged();
}

QList<MacroAction> MacroEditor::actions() const
{
    return m_actions;
}

void MacroEditor::addAction(const MacroAction& action)
{
    int idx = m_actions.size();
    m_actions.append(action);

    auto* item = new QListWidgetItem();
    item->setData(Qt::UserRole, idx);
    m_actionList->addItem(item);
    updateListItem(idx);

    // Выбираем новое действие
    m_actionList->setCurrentRow(idx);
    emit actionsChanged();
}

void MacroEditor::insertAction(int index, const MacroAction& action)
{
    if (index < 0) index = 0;
    if (index > m_actions.size()) index = m_actions.size();

    m_actions.insert(index, action);

    // Пересобираем список
    setActions(m_actions);
    m_actionList->setCurrentRow(index);
}

void MacroEditor::removeAction(int index)
{
    if (index < 0 || index >= m_actions.size()) return;

    m_actions.removeAt(index);
    setActions(m_actions);

    if (m_actions.isEmpty()) {
        m_paramsStack->setCurrentIndex(PageEmpty);
    } else {
        int newIdx = qMin(index, m_actions.size() - 1);
        m_actionList->setCurrentRow(newIdx);
    }
}

void MacroEditor::updateAction(int index, const MacroAction& action)
{
    if (index < 0 || index >= m_actions.size()) return;
    m_actions[index] = action;
    updateListItem(index);
    emit actionsChanged();
}

void MacroEditor::clearActions()
{
    m_actions.clear();
    m_actionList->clear();
    m_paramsStack->setCurrentIndex(PageEmpty);
    emit actionsChanged();
}

int MacroEditor::actionCount() const
{
    return m_actions.size();
}

void MacroEditor::moveActionUp(int index)
{
    if (index <= 0 || index >= m_actions.size()) return;
    m_actions.swapItemsAt(index, index - 1);
    setActions(m_actions);
    m_actionList->setCurrentRow(index - 1);
}

void MacroEditor::moveActionDown(int index)
{
    if (index < 0 || index >= m_actions.size() - 1) return;
    m_actions.swapItemsAt(index, index + 1);
    setActions(m_actions);
    m_actionList->setCurrentRow(index + 1);
}

int MacroEditor::selectedIndex() const
{
    return m_actionList->currentRow();
}

void MacroEditor::selectAction(int index)
{
    if (index >= 0 && index < m_actionList->count()) {
        m_actionList->setCurrentRow(index);
    }
}

void MacroEditor::highlightAction(int index)
{
    // Снимаем предыдущую подсветку
    if (m_highlightedIndex >= 0 && m_highlightedIndex < m_actionList->count()) {
        m_actionList->item(m_highlightedIndex)->setBackground(QBrush());
    }

    m_highlightedIndex = index;

    if (index >= 0 && index < m_actionList->count()) {
        m_actionList->item(index)->setBackground(QColor(100, 200, 100, 80));
        m_actionList->scrollToItem(m_actionList->item(index));
    }
}

void MacroEditor::clearHighlight()
{
    highlightAction(-1);
}

// ===========================================
// Слоты
// ===========================================

void MacroEditor::onItemSelectionChanged()
{
    int idx = m_actionList->currentRow();
    if (idx >= 0 && idx < m_actions.size()) {
        loadActionParams(idx);
        emit actionSelected(idx);
    } else {
        m_paramsStack->setCurrentIndex(PageEmpty);
    }
}

void MacroEditor::onRemoveSelected()
{
    int idx = selectedIndex();
    if (idx >= 0) {
        removeAction(idx);
    }
}

void MacroEditor::onMoveUp()
{
    moveActionUp(selectedIndex());
}

void MacroEditor::onMoveDown()
{
    moveActionDown(selectedIndex());
}

void MacroEditor::onDuplicateSelected()
{
    int idx = selectedIndex();
    if (idx >= 0 && idx < m_actions.size()) {
        MacroAction copy = m_actions[idx];
        insertAction(idx + 1, copy);
    }
}

void MacroEditor::onEditApplied()
{
    int idx = selectedIndex();
    if (idx < 0 || idx >= m_actions.size()) return;

    MacroAction& action = m_actions[idx];

    // Читаем параметры из текущей страницы
    switch (pageForType(action.type)) {
        case PageMouse:
            action.mouseButton  = static_cast<MacroMouseButton>(m_mouseButtonCombo->currentData().toInt());
            action.useCurrentPos = m_useCurrentPosCheck->isChecked();
            action.position     = QPoint(m_posXSpin->value(), m_posYSpin->value());
            break;

        case PageKeyboard:
            action.vkCode   = m_vkCodeSpin->value();
            action.withCtrl = m_ctrlCheck->isChecked();
            action.withShift = m_shiftCheck->isChecked();
            action.withAlt  = m_altCheck->isChecked();
            action.withWin  = m_winCheck->isChecked();
            break;

        case PageTypeText:
            action.text    = m_textEdit->toPlainText();
            action.delayMs = m_typeDelaySpin->value();
            break;

        case PageDelay:
            action.delayMs = m_delaySpin->value();
            break;

        case PageRandomDelay:
            action.delayMinMs = m_delayMinSpin->value();
            action.delayMaxMs = m_delayMaxSpin->value();
            break;

        case PageLoop:
            action.loopCount = m_loopCountSpin->value();
            break;

        case PageSubMacro:
            action.subMacroName = m_subMacroEdit->text().trimmed();
            break;

        case PageComment:
            action.comment = m_commentEdit->text().trimmed();
            break;

        case PageMouseMove:
            action.position = QPoint(m_movePosXSpin->value(), m_movePosYSpin->value());
            break;

        default:
            break;
    }

    updateListItem(idx);
    emit actionsChanged();
}

void MacroEditor::onAddActionMenu()
{
    if (m_addMenu) {
        m_addMenu->exec(QCursor::pos());
    }
}

// ===========================================
// Обновление элемента списка
// ===========================================

void MacroEditor::updateListItem(int index)
{
    if (index < 0 || index >= m_actionList->count()) return;
    if (index >= m_actions.size()) return;

    QListWidgetItem* item = m_actionList->item(index);
    const MacroAction& action = m_actions[index];

    // Форматируем текст
    QString prefix;
    // Отступ для действий внутри циклов
    int loopDepth = 0;
    for (int i = 0; i < index; ++i) {
        if (m_actions[i].type == MacroActionType::LoopStart) loopDepth++;
        if (m_actions[i].type == MacroActionType::LoopEnd) loopDepth--;
    }
    if (action.type == MacroActionType::LoopEnd && loopDepth > 0) loopDepth--;
    for (int i = 0; i < loopDepth; ++i) prefix += "    ";

    QString text = prefix + QString("%1. %2").arg(index + 1).arg(action.description());
    item->setText(text);

    // Цвет
    item->setForeground(QColor(actionColor(action.type)));
}

// ===========================================
// Загрузка параметров действия в панель
// ===========================================

void MacroEditor::loadActionParams(int index)
{
    if (index < 0 || index >= m_actions.size()) return;

    const MacroAction& action = m_actions[index];
    ParamPage page = pageForType(action.type);
    m_paramsStack->setCurrentIndex(page);

    switch (page) {
        case PageMouse:
            m_mouseButtonCombo->setCurrentIndex(static_cast<int>(action.mouseButton));
            m_useCurrentPosCheck->setChecked(action.useCurrentPos);
            m_posXSpin->setValue(action.position.x());
            m_posYSpin->setValue(action.position.y());
            m_posXSpin->setEnabled(!action.useCurrentPos);
            m_posYSpin->setEnabled(!action.useCurrentPos);
            break;

        case PageKeyboard:
            m_vkCodeSpin->setValue(action.vkCode);
            m_keyNameLabel->setText(KeyboardClicker::vkCodeToName(action.vkCode));
            m_ctrlCheck->setChecked(action.withCtrl);
            m_shiftCheck->setChecked(action.withShift);
            m_altCheck->setChecked(action.withAlt);
            m_winCheck->setChecked(action.withWin);
            // Показываем/скрываем модификаторы для KeyCombo
            {
                bool showMods = (action.type == MacroActionType::KeyCombo);
                m_ctrlCheck->setVisible(showMods);
                m_shiftCheck->setVisible(showMods);
                m_altCheck->setVisible(showMods);
                m_winCheck->setVisible(showMods);
            }
            break;

        case PageTypeText:
            m_textEdit->setPlainText(action.text);
            m_typeDelaySpin->setValue(action.delayMs);
            break;

        case PageDelay:
            m_delaySpin->setValue(action.delayMs);
            break;

        case PageRandomDelay:
            m_delayMinSpin->setValue(action.delayMinMs);
            m_delayMaxSpin->setValue(action.delayMaxMs);
            break;

        case PageLoop:
            m_loopCountSpin->setValue(action.loopCount);
            break;

        case PageSubMacro:
            m_subMacroEdit->setText(action.subMacroName);
            break;

        case PageComment:
            m_commentEdit->setText(action.comment);
            break;

        case PageMouseMove:
            m_movePosXSpin->setValue(action.position.x());
            m_movePosYSpin->setValue(action.position.y());
            break;

        default:
            break;
    }
}

// ===========================================
// Определение страницы параметров по типу
// ===========================================

MacroEditor::ParamPage MacroEditor::pageForType(MacroActionType type) const
{
    switch (type) {
        case MacroActionType::MouseClick:
        case MacroActionType::MouseDoubleClick:
        case MacroActionType::MouseDown:
        case MacroActionType::MouseUp:
            return PageMouse;

        case MacroActionType::MouseMove:
            return PageMouseMove;

        case MacroActionType::KeyDown:
        case MacroActionType::KeyUp:
        case MacroActionType::KeyPress:
        case MacroActionType::KeyCombo:
            return PageKeyboard;

        case MacroActionType::TypeText:
            return PageTypeText;

        case MacroActionType::Delay:
            return PageDelay;

        case MacroActionType::RandomDelay:
            return PageRandomDelay;

        case MacroActionType::LoopStart:
            return PageLoop;

        case MacroActionType::LoopEnd:
            return PageEmpty;  // Нет параметров

        case MacroActionType::SubMacro:
            return PageSubMacro;

        case MacroActionType::Comment:
            return PageComment;

        default:
            return PageEmpty;
    }
}

// ===========================================
// Создание действия по умолчанию
// ===========================================

MacroAction MacroEditor::createDefaultAction(MacroActionType type) const
{
    MacroAction action;
    action.type = type;

    switch (type) {
        case MacroActionType::MouseClick:
        case MacroActionType::MouseDoubleClick:
            action.mouseButton  = MacroMouseButton::Left;
            action.useCurrentPos = true;
            break;

        case MacroActionType::MouseDown:
        case MacroActionType::MouseUp:
            action.mouseButton  = MacroMouseButton::Left;
            action.useCurrentPos = true;
            break;

        case MacroActionType::MouseMove:
            action.position = QPoint(0, 0);
            break;

        case MacroActionType::KeyPress:
        case MacroActionType::KeyDown:
        case MacroActionType::KeyUp:
            action.vkCode = 0x41;  // 'A' по умолчанию
            break;

        case MacroActionType::KeyCombo:
            action.vkCode   = 0x56;  // 'V'
            action.withCtrl = true;
            break;

        case MacroActionType::TypeText:
            action.text    = tr("Hello");
            action.delayMs = 30;
            break;

        case MacroActionType::Delay:
            action.delayMs = 100;
            break;

        case MacroActionType::RandomDelay:
            action.delayMinMs = 50;
            action.delayMaxMs = 200;
            break;

        case MacroActionType::LoopStart:
            action.loopCount = 3;
            break;

        case MacroActionType::SubMacro:
            action.subMacroName = "";
            break;

        case MacroActionType::Comment:
            action.comment = tr("Комментарий");
            break;

        default:
            break;
    }

    return action;
}

void MacroEditor::addActionOfType(MacroActionType type)
{
    MacroAction action = createDefaultAction(type);

    // Вставляем после выделенного или в конец
    int idx = selectedIndex();
    if (idx >= 0) {
        insertAction(idx + 1, action);
    } else {
        addAction(action);
    }
}

// ===========================================
// Иконки и цвета
// ===========================================

QIcon MacroEditor::actionIcon(MacroActionType type) const
{
    Q_UNUSED(type);
    return QIcon();  // Пока без иконок — используем цветные надписи
}

QString MacroEditor::actionColor(MacroActionType type) const
{
    switch (type) {
        case MacroActionType::MouseClick:
        case MacroActionType::MouseDoubleClick:
        case MacroActionType::MouseDown:
        case MacroActionType::MouseUp:
        case MacroActionType::MouseMove:
            return "#4FC3F7";  // Голубой — мышь

        case MacroActionType::KeyDown:
        case MacroActionType::KeyUp:
        case MacroActionType::KeyPress:
        case MacroActionType::KeyCombo:
        case MacroActionType::TypeText:
            return "#81C784";  // Зелёный — клавиатура

        case MacroActionType::Delay:
        case MacroActionType::RandomDelay:
            return "#FFB74D";  // Оранжевый — задержки

        case MacroActionType::LoopStart:
        case MacroActionType::LoopEnd:
            return "#CE93D8";  // Фиолетовый — циклы

        case MacroActionType::SubMacro:
            return "#F48FB1";  // Розовый — вложенные

        case MacroActionType::Comment:
            return "#90A4AE";  // Серый — комментарии

        default:
            return "#FFFFFF";
    }
}

// ===========================================
// Перехват клавиш для режима захвата
// ===========================================

bool MacroEditor::eventFilter(QObject* obj, QEvent* event)
{
    if (m_capturing && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        // Пропускаем модификаторы отдельно
        int key = keyEvent->key();
        if (key == Qt::Key_Control || key == Qt::Key_Shift ||
            key == Qt::Key_Alt || key == Qt::Key_Meta) {
            return true;  // Ждём настоящую клавишу
        }

        // Конвертируем Qt::Key → VK-код
        int vk = keyEvent->nativeVirtualKey();
        if (vk > 0) {
            m_vkCodeSpin->setValue(vk);
            m_keyNameLabel->setText(KeyboardClicker::vkCodeToName(vk));
            m_capturedVk = vk;
        }

        m_capturing = false;
        m_captureKeyBtn->setText(tr("🎯 Захватить клавишу"));
        return true;
    }

    return QWidget::eventFilter(obj, event);
}
