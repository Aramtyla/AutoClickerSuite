// ===========================================
// MacroWidget.cpp — Полноценный GUI вкладки макросов
// Запись, воспроизведение, визуальный редактор,
// экспорт/импорт JSON, управление библиотекой
// ===========================================

#include "MacroWidget.h"
#include "MacroEditor.h"
#include "MacroRecorder.h"
#include "MacroPlayer.h"
#include "MacroConfig.h"

#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>

MacroWidget::MacroWidget(QWidget* parent)
    : QWidget(parent)
{
    m_recorder = new MacroRecorder(this);
    m_player   = new MacroPlayer(this);
    m_editor   = new MacroEditor(this);

    setupUI();
    connectSignals();
    loadMacrosFromDisk();
    updateToolbarState();
}

MacroRecorder* MacroWidget::recorder() const { return m_recorder; }
MacroPlayer*   MacroWidget::player() const { return m_player; }

// ===========================================
// Управление из MainWindow / хоткеев
// ===========================================

void MacroWidget::toggleStartStop()
{
    if (m_player->isPlaying()) {
        onStop();
    } else {
        onPlay();
    }
}

void MacroWidget::toggleRecord()
{
    onRecord();
}

void MacroWidget::emergencyStop()
{
    if (m_player->isPlaying()) {
        m_player->stop();
    }
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    }
}

bool MacroWidget::playMacroByName(const QString& name)
{
    if (m_player->isPlaying()) {
        m_player->stop();
    }
    if (!m_macros.contains(name)) {
        return false;
    }

    // Выбираем макрос в списке
    for (int i = 0; i < m_macroList->count(); ++i) {
        if (m_macroList->item(i)->text() == name) {
            m_macroList->setCurrentRow(i);
            break;
        }
    }

    m_currentMacroName = name;
    Macro& macro = m_macros[name];
    macro.repeatCount     = m_repeatSpin->value();
    macro.speedMultiplier = m_speedSpin->value();

    m_player->setMacroLibrary(m_macros);
    m_player->setSpeedMultiplier(m_speedSpin->value());
    m_player->play(macro);
    return true;
}

// ===========================================
// Построение интерфейса
// ===========================================

void MacroWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ==========================================
    // Панель инструментов сверху
    // ==========================================
    setupToolbar();
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->addWidget(m_btnRecord);
    toolbarLayout->addWidget(m_btnPlay);
    toolbarLayout->addWidget(m_btnPause);
    toolbarLayout->addWidget(m_btnStop);
    toolbarLayout->addStretch();
    mainLayout->addLayout(toolbarLayout);

    // ==========================================
    // Главный сплиттер: слева список, справа редактор + настройки
    // ==========================================
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // --- Левая панель: список макросов ---
    auto* leftPanel = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    setupMacroList();

    auto* macroListGroup = new QGroupBox(tr("Библиотека макросов"), leftPanel);
    auto* macroListLayout = new QVBoxLayout(macroListGroup);
    macroListLayout->addWidget(m_macroList);

    auto* listBtnLayout = new QHBoxLayout();
    listBtnLayout->addWidget(m_btnNewMacro);
    listBtnLayout->addWidget(m_btnDeleteMacro);
    listBtnLayout->addWidget(m_btnRenameMacro);
    macroListLayout->addLayout(listBtnLayout);

    auto* ioBtnLayout = new QHBoxLayout();
    ioBtnLayout->addWidget(m_btnImport);
    ioBtnLayout->addWidget(m_btnExport);
    macroListLayout->addLayout(ioBtnLayout);

    leftLayout->addWidget(macroListGroup);

    // --- Настройки записи ---
    setupSettings();
    auto* settingsGroup = new QGroupBox(tr("Настройки"), leftPanel);
    auto* settingsLayout = new QVBoxLayout(settingsGroup);

    auto* recGroup = new QGroupBox(tr("Запись"), settingsGroup);
    auto* recLayout = new QVBoxLayout(recGroup);
    recLayout->addWidget(m_recMouseCheck);
    recLayout->addWidget(m_recKeyboardCheck);
    recLayout->addWidget(m_recMouseMoveCheck);
    settingsLayout->addWidget(recGroup);

    auto* playGroup = new QGroupBox(tr("Воспроизведение"), settingsGroup);
    auto* playLayout = new QFormLayout(playGroup);
    playLayout->addRow(tr("Повторений:"), m_repeatSpin);
    playLayout->addRow(tr("Скорость:"), m_speedSpin);
    settingsLayout->addWidget(playGroup);

    leftLayout->addWidget(settingsGroup);

    // --- Правая панель: редактор + статистика ---
    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* editorGroup = new QGroupBox(tr("Редактор действий"), rightPanel);
    auto* editorLayout = new QVBoxLayout(editorGroup);
    editorLayout->addWidget(m_editor);
    rightLayout->addWidget(editorGroup, 1);

    // Статистика
    setupStats();
    auto* statsGroup = new QGroupBox(tr("Статистика"), rightPanel);
    auto* statsLayout = new QVBoxLayout(statsGroup);
    statsLayout->addWidget(m_statusLabel);
    statsLayout->addWidget(m_progressBar);
    auto* statsRowLayout = new QHBoxLayout();
    statsRowLayout->addWidget(m_repeatLabel);
    statsRowLayout->addStretch();
    statsRowLayout->addWidget(m_actionCountLabel);
    statsLayout->addLayout(statsRowLayout);
    rightLayout->addWidget(statsGroup);

    // Пропорции сплиттера: 30% список, 70% редактор
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 7);

    mainLayout->addWidget(splitter, 1);
}

void MacroWidget::setupToolbar()
{
    m_btnRecord = new QPushButton(tr("⏺ Записать (F7)"), this);
    m_btnPlay   = new QPushButton(tr("▶ Воспроизвести (F6)"), this);
    m_btnPause  = new QPushButton(tr("⏸ Пауза"), this);
    m_btnStop   = new QPushButton(tr("⏹ Стоп"), this);

    m_btnRecord->setCheckable(true);
    m_btnPause->setEnabled(false);
    m_btnStop->setEnabled(false);

    m_btnRecord->setToolTip(tr("Начать/остановить запись макроса (F7)"));
    m_btnPlay->setToolTip(tr("Воспроизвести текущий макрос (F6)"));
    m_btnPause->setToolTip(tr("Приостановить/возобновить воспроизведение"));
    m_btnStop->setToolTip(tr("Остановить воспроизведение"));

    connect(m_btnRecord, &QPushButton::clicked, this, &MacroWidget::onRecord);
    connect(m_btnPlay,   &QPushButton::clicked, this, &MacroWidget::onPlay);
    connect(m_btnPause,  &QPushButton::clicked, this, &MacroWidget::onPause);
    connect(m_btnStop,   &QPushButton::clicked, this, &MacroWidget::onStop);
}

void MacroWidget::setupMacroList()
{
    m_macroList = new QListWidget(this);
    m_macroList->setMaximumHeight(200);

    m_btnNewMacro    = new QPushButton(tr("➕ Новый"), this);
    m_btnDeleteMacro = new QPushButton(tr("🗑 Удалить"), this);
    m_btnRenameMacro = new QPushButton(tr("✏ Переименовать"), this);
    m_btnImport      = new QPushButton(tr("📥 Импорт"), this);
    m_btnExport      = new QPushButton(tr("📤 Экспорт"), this);

    connect(m_btnNewMacro,    &QPushButton::clicked, this, &MacroWidget::onNewMacro);
    connect(m_btnDeleteMacro, &QPushButton::clicked, this, &MacroWidget::onDeleteMacro);
    connect(m_btnRenameMacro, &QPushButton::clicked, this, &MacroWidget::onRenameMacro);
    connect(m_btnImport,      &QPushButton::clicked, this, &MacroWidget::onImportMacro);
    connect(m_btnExport,      &QPushButton::clicked, this, &MacroWidget::onExportMacro);
    connect(m_macroList, &QListWidget::currentRowChanged,
            this, &MacroWidget::onMacroListSelectionChanged);
}

void MacroWidget::setupSettings()
{
    m_recMouseCheck    = new QCheckBox(tr("Записывать мышь"), this);
    m_recKeyboardCheck = new QCheckBox(tr("Записывать клавиатуру"), this);
    m_recMouseMoveCheck = new QCheckBox(tr("Записывать перемещения"), this);

    m_recMouseCheck->setChecked(true);
    m_recKeyboardCheck->setChecked(true);
    m_recMouseMoveCheck->setChecked(true);

    m_repeatSpin = new QSpinBox(this);
    m_repeatSpin->setRange(0, AppConstants::Macro::MAX_LOOP_COUNT);
    m_repeatSpin->setSpecialValueText(tr("∞ (бесконечно)"));
    m_repeatSpin->setValue(1);

    m_speedSpin = new QDoubleSpinBox(this);
    m_speedSpin->setRange(0.1, 10.0);
    m_speedSpin->setSingleStep(0.1);
    m_speedSpin->setSuffix("x");
    m_speedSpin->setValue(1.0);

    connect(m_recMouseCheck, &QCheckBox::toggled, this, &MacroWidget::onSettingsChanged);
    connect(m_recKeyboardCheck, &QCheckBox::toggled, this, &MacroWidget::onSettingsChanged);
    connect(m_recMouseMoveCheck, &QCheckBox::toggled, this, &MacroWidget::onSettingsChanged);
}

void MacroWidget::setupStats()
{
    m_statusLabel     = new QLabel(tr("Статус: Готов"), this);
    m_progressBar     = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_repeatLabel     = new QLabel(tr("Повтор: —"), this);
    m_actionCountLabel = new QLabel(tr("Действий: 0"), this);
}

// ===========================================
// Подключение сигналов
// ===========================================

void MacroWidget::connectSignals()
{
    // === Recorder ===
    connect(m_recorder, &MacroRecorder::recordingStarted,
            this, &MacroWidget::onRecordingStarted);
    connect(m_recorder, &MacroRecorder::recordingStopped,
            this, &MacroWidget::onRecordingStopped);
    connect(m_recorder, &MacroRecorder::actionRecorded,
            this, &MacroWidget::onActionRecorded);

    // === Player ===
    connect(m_player, &MacroPlayer::playbackStarted,
            this, &MacroWidget::onPlaybackStarted);
    connect(m_player, &MacroPlayer::playbackStopped,
            this, &MacroWidget::onPlaybackStopped);
    connect(m_player, &MacroPlayer::playbackFinished,
            this, &MacroWidget::onPlaybackFinished);
    connect(m_player, &MacroPlayer::progressChanged,
            this, &MacroWidget::onPlaybackProgress);
    connect(m_player, &MacroPlayer::repeatChanged,
            this, &MacroWidget::onPlaybackRepeat);
    connect(m_player, &MacroPlayer::actionExecuted,
            this, &MacroWidget::onActionExecuted);

    // === Editor: изменения редактора → сохранение в текущий макрос ===
    connect(m_editor, &MacroEditor::actionsChanged, this, [this]() {
        saveMacroToList();
        m_actionCountLabel->setText(tr("Действий: %1").arg(m_editor->actionCount()));
    });
}

// ===========================================
// Управление записью
// ===========================================

void MacroWidget::onRecord()
{
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    } else {
        // Если нет текущего макроса — создаём новый
        if (m_currentMacroName.isEmpty()) {
            onNewMacro();
            if (m_currentMacroName.isEmpty()) return;  // Отменил
        }

        // Применяем настройки записи
        m_recorder->setRecordMouse(m_recMouseCheck->isChecked());
        m_recorder->setRecordKeyboard(m_recKeyboardCheck->isChecked());
        m_recorder->setRecordMouseMove(m_recMouseMoveCheck->isChecked());

        // Очищаем редактор перед записью
        m_editor->clearActions();

        m_recorder->startRecording();
    }
}

void MacroWidget::onPlay()
{
    if (m_player->isPlaying()) return;
    if (m_currentMacroName.isEmpty() || !m_macros.contains(m_currentMacroName)) {
        LOG_WARNING(tr("Нет макроса для воспроизведения"));
        return;
    }

    // Сохраняем текущие настройки в макрос
    saveMacroToList();

    Macro& macro = m_macros[m_currentMacroName];
    macro.repeatCount     = m_repeatSpin->value();
    macro.speedMultiplier = m_speedSpin->value();

    // Задаём библиотеку для вложенных макросов
    m_player->setMacroLibrary(m_macros);
    m_player->setSpeedMultiplier(m_speedSpin->value());
    m_player->play(macro);
}

void MacroWidget::onStop()
{
    if (m_player->isPlaying()) {
        m_player->stop();
    }
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    }
}

void MacroWidget::onPause()
{
    if (m_player->isPaused()) {
        m_player->resume();
        m_btnPause->setText(tr("⏸ Пауза"));
    } else if (m_player->isPlaying()) {
        m_player->pause();
        m_btnPause->setText(tr("▶ Продолжить"));
    }
}

// ===========================================
// Список макросов
// ===========================================

void MacroWidget::onNewMacro()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Новый макрос"),
                                          tr("Имя макроса:"),
                                          QLineEdit::Normal,
                                          tr("Макрос %1").arg(m_macros.size() + 1),
                                          &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    if (m_macros.contains(name)) {
        QMessageBox::warning(this, tr("Ошибка"),
                              tr("Макрос с именем '%1' уже существует").arg(name));
        return;
    }

    Macro macro;
    macro.name       = name;
    macro.createdAt  = QDateTime::currentDateTime().toString(Qt::ISODate);
    macro.modifiedAt = macro.createdAt;
    macro.repeatCount = m_repeatSpin->value();
    macro.speedMultiplier = m_speedSpin->value();

    m_macros.insert(name, macro);
    m_macroList->addItem(name);
    m_macroList->setCurrentRow(m_macroList->count() - 1);

    m_currentMacroName = name;
    m_editor->clearActions();

    LOG_INFO(tr("Создан новый макрос: '%1'").arg(name));
    updateToolbarState();
}

void MacroWidget::onDeleteMacro()
{
    if (m_currentMacroName.isEmpty()) return;

    int ret = QMessageBox::question(this, tr("Удаление макроса"),
        tr("Удалить макрос '%1'?").arg(m_currentMacroName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    // Удаляем файл
    QString path = macrosDir() + "/" + m_currentMacroName + ".json";
    QFile::remove(path);

    m_macros.remove(m_currentMacroName);
    delete m_macroList->currentItem();

    m_currentMacroName.clear();
    m_editor->clearActions();

    LOG_INFO(tr("Макрос удалён"));
    updateToolbarState();
}

void MacroWidget::onRenameMacro()
{
    if (m_currentMacroName.isEmpty()) return;

    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Переименование"),
                                             tr("Новое имя:"),
                                             QLineEdit::Normal,
                                             m_currentMacroName, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    newName = newName.trimmed();

    if (m_macros.contains(newName)) {
        QMessageBox::warning(this, tr("Ошибка"),
                              tr("Макрос с именем '%1' уже существует").arg(newName));
        return;
    }

    // Удаляем старый файл
    QFile::remove(macrosDir() + "/" + m_currentMacroName + ".json");

    Macro macro = m_macros.take(m_currentMacroName);
    macro.name = newName;
    macro.modifiedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_macros.insert(newName, macro);

    // Обновляем список
    if (auto* item = m_macroList->currentItem()) {
        item->setText(newName);
    }

    m_currentMacroName = newName;
    saveMacroToFile(macro);

    LOG_INFO(tr("Макрос переименован в '%1'").arg(newName));
}

void MacroWidget::onMacroListSelectionChanged()
{
    int row = m_macroList->currentRow();
    if (row < 0) {
        m_currentMacroName.clear();
        m_editor->clearActions();
        updateToolbarState();
        return;
    }

    // Сначала сохраняем текущий макрос
    if (!m_currentMacroName.isEmpty() && m_macros.contains(m_currentMacroName)) {
        saveMacroToList();
    }

    // Загружаем выбранный
    QString name = m_macroList->item(row)->text();
    m_currentMacroName = name;

    if (m_macros.contains(name)) {
        m_editor->setActions(m_macros[name].actions);
        m_repeatSpin->setValue(m_macros[name].repeatCount);
        m_speedSpin->setValue(m_macros[name].speedMultiplier);
        m_actionCountLabel->setText(tr("Действий: %1").arg(m_macros[name].actions.size()));
    }

    updateToolbarState();
}

// ===========================================
// Импорт / Экспорт
// ===========================================

void MacroWidget::onImportMacro()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Импорт макросов"), QString(),
        tr("Макросы JSON (*.json);;Все файлы (*)"));

    for (const QString& filePath : files) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR(tr("Не удалось открыть файл: %1").arg(filePath));
            continue;
        }

        QByteArray fileData = file.readAll();
        file.close();

        Macro macro = Macro::fromJsonBytes(fileData);
        if (macro.name.isEmpty()) {
            // Используем имя файла
            QFileInfo fi(filePath);
            macro.name = fi.baseName();
        }

        // Уникализация имени
        QString baseName = macro.name;
        int counter = 1;
        while (m_macros.contains(macro.name)) {
            macro.name = QString("%1 (%2)").arg(baseName).arg(counter++);
        }

        m_macros.insert(macro.name, macro);
        m_macroList->addItem(macro.name);
        saveMacroToFile(macro);

        LOG_INFO(tr("Импортирован макрос: '%1' (%2 действий)")
                     .arg(macro.name).arg(macro.actions.size()));
    }
}

void MacroWidget::onExportMacro()
{
    if (m_currentMacroName.isEmpty()) return;

    saveMacroToList();
    const Macro& macro = m_macros[m_currentMacroName];

    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Экспорт макроса"),
        m_currentMacroName + ".json",
        tr("Макросы JSON (*.json)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(tr("Не удалось сохранить файл: %1").arg(filePath));
        return;
    }

    file.write(macro.toJsonBytes());
    file.close();

    LOG_INFO(tr("Макрос '%1' экспортирован в: %2").arg(macro.name, filePath));
}

// ===========================================
// Обработчики записи
// ===========================================

void MacroWidget::onRecordingStarted()
{
    m_btnRecord->setChecked(true);
    m_btnRecord->setText(tr("⏹ Остановить запись"));
    m_btnPlay->setEnabled(false);
    m_statusLabel->setText(tr("Статус: 🔴 Запись..."));
    m_progressBar->setRange(0, 0);  // Индикатор бесконечной активности
}

void MacroWidget::onRecordingStopped()
{
    m_btnRecord->setChecked(false);
    m_btnRecord->setText(tr("⏺ Записать (F7)"));
    m_btnPlay->setEnabled(true);
    m_statusLabel->setText(tr("Статус: Запись завершена"));
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    // Перенести записанные действия в редактор
    QList<MacroAction> actions = m_recorder->recordedActions();
    m_editor->setActions(actions);
    m_actionCountLabel->setText(tr("Действий: %1").arg(actions.size()));

    // Сохраняем в текущий макрос
    saveMacroToList();

    updateToolbarState();
}

void MacroWidget::onActionRecorded(const MacroAction& action)
{
    Q_UNUSED(action)
    // Живое отображение записанных действий
    m_actionCountLabel->setText(tr("Действий: %1").arg(m_recorder->actionCount()));
}

// ===========================================
// Обработчики воспроизведения
// ===========================================

void MacroWidget::onPlaybackStarted()
{
    m_btnPlay->setEnabled(false);
    m_btnPause->setEnabled(true);
    m_btnStop->setEnabled(true);
    m_btnRecord->setEnabled(false);
    m_statusLabel->setText(tr("Статус: ▶ Воспроизведение..."));
    emit macroStarted();
}

void MacroWidget::onPlaybackStopped()
{
    m_btnPlay->setEnabled(true);
    m_btnPause->setEnabled(false);
    m_btnStop->setEnabled(false);
    m_btnRecord->setEnabled(true);
    m_btnPause->setText(tr("⏸ Пауза"));
    m_progressBar->setValue(0);
    m_editor->clearHighlight();
    updateToolbarState();
    emit macroStopped();
}

void MacroWidget::onPlaybackFinished()
{
    m_statusLabel->setText(tr("Статус: ✅ Воспроизведение завершено"));
    m_progressBar->setValue(100);
    LOG_INFO(tr("Макрос полностью воспроизведён"));
}

void MacroWidget::onPlaybackProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setRange(0, total);
        m_progressBar->setValue(current);
    }
}

void MacroWidget::onPlaybackRepeat(int current, int total)
{
    if (total > 0) {
        m_repeatLabel->setText(tr("Повтор: %1 / %2").arg(current).arg(total));
    } else {
        m_repeatLabel->setText(tr("Повтор: %1 / ∞").arg(current));
    }
}

void MacroWidget::onActionExecuted(int index, const MacroAction& action)
{
    Q_UNUSED(action);
    m_editor->highlightAction(index);
}

// ===========================================
// Настройки
// ===========================================

void MacroWidget::onSettingsChanged()
{
    m_recorder->setRecordMouse(m_recMouseCheck->isChecked());
    m_recorder->setRecordKeyboard(m_recKeyboardCheck->isChecked());
    m_recorder->setRecordMouseMove(m_recMouseMoveCheck->isChecked());
}

// ===========================================
// Вспомогательные
// ===========================================

void MacroWidget::saveMacroToList()
{
    if (m_currentMacroName.isEmpty()) return;

    if (m_macros.contains(m_currentMacroName)) {
        Macro& macro = m_macros[m_currentMacroName];
        macro.actions       = m_editor->actions();
        macro.repeatCount   = m_repeatSpin->value();
        macro.speedMultiplier = m_speedSpin->value();
        macro.modifiedAt    = QDateTime::currentDateTime().toString(Qt::ISODate);

        saveMacroToFile(macro);
    }
}

void MacroWidget::loadMacroFromList(int index)
{
    if (index < 0 || index >= m_macroList->count()) return;

    QString name = m_macroList->item(index)->text();
    if (m_macros.contains(name)) {
        m_currentMacroName = name;
        m_editor->setActions(m_macros[name].actions);
    }
}

void MacroWidget::updateMacroLibrary()
{
    m_player->setMacroLibrary(m_macros);
}

void MacroWidget::updateToolbarState()
{
    bool hasMacro = !m_currentMacroName.isEmpty();
    bool hasActions = hasMacro && m_macros.contains(m_currentMacroName) &&
                      !m_macros[m_currentMacroName].actions.isEmpty();
    bool isPlaying = m_player->isPlaying();
    bool isRecording = m_recorder->isRecording();

    m_btnPlay->setEnabled(hasActions && !isPlaying && !isRecording);
    m_btnRecord->setEnabled(!isPlaying);
    m_btnPause->setEnabled(isPlaying);
    m_btnStop->setEnabled(isPlaying || isRecording);

    m_btnDeleteMacro->setEnabled(hasMacro && !isPlaying && !isRecording);
    m_btnRenameMacro->setEnabled(hasMacro && !isPlaying && !isRecording);
    m_btnExport->setEnabled(hasMacro);
}

// ===========================================
// Файловое хранение макросов
// ===========================================

QString MacroWidget::macrosDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/" + AppConstants::Paths::MACROS_DIR;
    QDir().mkpath(dir);
    return dir;
}

void MacroWidget::saveMacroToFile(const Macro& macro)
{
    if (macro.name.isEmpty()) return;

    QString path = macrosDir() + "/" + macro.name + ".json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(macro.toJsonBytes());
        file.close();
    }
}

void MacroWidget::loadMacrosFromDisk()
{
    QString dir = macrosDir();
    QDir macroDir(dir);
    QStringList files = macroDir.entryList({"*.json"}, QDir::Files);

    for (const QString& fileName : files) {
        QFile file(dir + "/" + fileName);
        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray fileData = file.readAll();
        file.close();

        Macro macro = Macro::fromJsonBytes(fileData);
        if (macro.name.isEmpty()) {
            macro.name = QFileInfo(fileName).baseName();
        }

        m_macros.insert(macro.name, macro);
        m_macroList->addItem(macro.name);
    }

    if (m_macroList->count() > 0) {
        m_macroList->setCurrentRow(0);
    }

    LOG_INFO(tr("Загружено макросов из диска: %1").arg(m_macros.size()));
}
