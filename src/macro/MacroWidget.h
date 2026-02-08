#pragma once
// ===========================================
// MacroWidget.h — Полноценный GUI вкладки макросов
// Включает: панель инструментов, визуальный редактор,
// список макросов, настройки, статистику
// ===========================================

#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QProgressBar>
#include <QLineEdit>
#include <QSplitter>

#include "MacroConfig.h"

class MacroEditor;
class MacroRecorder;
class MacroPlayer;

class MacroWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MacroWidget(QWidget* parent = nullptr);

    // Внешний доступ к компонентам (для MainWindow)
    MacroRecorder* recorder() const;
    MacroPlayer*   player() const;

    // Управление из MainWindow / хоткеев
    void toggleStartStop();
    void toggleRecord();
    void emergencyStop();

    // Запуск конкретного макроса по имени (для планировщика)
    bool playMacroByName(const QString& name);

signals:
    void macroStarted();
    void macroStopped();

private slots:
    // === Управление ===
    void onRecord();
    void onPlay();
    void onStop();
    void onPause();

    // === Список макросов ===
    void onNewMacro();
    void onDeleteMacro();
    void onRenameMacro();
    void onMacroListSelectionChanged();

    // === Импорт/экспорт ===
    void onImportMacro();
    void onExportMacro();

    // === Запись ===
    void onRecordingStarted();
    void onRecordingStopped();
    void onActionRecorded(const MacroAction& action);

    // === Воспроизведение ===
    void onPlaybackStarted();
    void onPlaybackStopped();
    void onPlaybackFinished();
    void onPlaybackProgress(int current, int total);
    void onPlaybackRepeat(int current, int total);
    void onActionExecuted(int index, const MacroAction& action);

    // === Настройки ===
    void onSettingsChanged();

private:
    void setupUI();
    void setupToolbar();
    void setupMacroList();
    void setupEditor();
    void setupSettings();
    void setupStats();
    void connectSignals();

    // Вспомогательные
    void saveMacroToList();
    void loadMacroFromList(int index);
    void updateMacroLibrary();
    void updateToolbarState();

    // Получить каталог макросов
    QString macrosDir() const;

    // Автосохранение макроса в файл
    void saveMacroToFile(const Macro& macro);
    void loadMacrosFromDisk();

    // ==========================================
    // Движки
    // ==========================================
    MacroRecorder*    m_recorder     = nullptr;
    MacroPlayer*      m_player       = nullptr;
    MacroEditor*      m_editor       = nullptr;

    // ==========================================
    // Список макросов
    // ==========================================
    QMap<QString, Macro> m_macros;           // Все макросы (имя → макрос)
    QString              m_currentMacroName; // Имя текущего макроса

    // ==========================================
    // Виджеты — Тулбар
    // ==========================================
    QPushButton*      m_btnRecord    = nullptr;
    QPushButton*      m_btnPlay      = nullptr;
    QPushButton*      m_btnPause     = nullptr;
    QPushButton*      m_btnStop      = nullptr;

    // ==========================================
    // Виджеты — Список макросов (слева)
    // ==========================================
    QListWidget*      m_macroList    = nullptr;
    QPushButton*      m_btnNewMacro  = nullptr;
    QPushButton*      m_btnDeleteMacro = nullptr;
    QPushButton*      m_btnRenameMacro = nullptr;
    QPushButton*      m_btnImport    = nullptr;
    QPushButton*      m_btnExport    = nullptr;

    // ==========================================
    // Настройки воспроизведения
    // ==========================================
    QSpinBox*         m_repeatSpin   = nullptr;
    QDoubleSpinBox*   m_speedSpin    = nullptr;
    QCheckBox*        m_recMouseCheck = nullptr;
    QCheckBox*        m_recKeyboardCheck = nullptr;
    QCheckBox*        m_recMouseMoveCheck = nullptr;

    // ==========================================
    // Статистика
    // ==========================================
    QLabel*           m_statusLabel  = nullptr;
    QProgressBar*     m_progressBar  = nullptr;
    QLabel*           m_repeatLabel  = nullptr;
    QLabel*           m_actionCountLabel = nullptr;
};
