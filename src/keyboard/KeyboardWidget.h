#pragma once
// ===========================================
// KeyboardWidget.h — Полный GUI модуля автонажатия клавиатуры
// 4 режима: одиночная клавиша, комбинация, текст, макрос
// ===========================================

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QStackedWidget>
#include <QTimer>
#include <QKeyEvent>

#include "KeyboardConfig.h"

class KeyboardClicker;

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget* parent = nullptr);
    ~KeyboardWidget();

    // Внешний доступ для MainWindow
    KeyboardClicker* clicker() const { return m_clicker; }

public slots:
    void toggleStartStop();

protected:
    // Перехват нажатий для захвата клавиши
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onStartClicked();
    void onStopClicked();
    void applyConfig();
    void onStatsUpdated(qint64 presses, qint64 elapsedMs);
    void onClickerStarted();
    void onClickerStopped();
    void onClickerFinished();
    void onModeChanged(int index);

    // Режим захвата клавиши
    void onCaptureKey();

    // Запись макроса
    void onRecordToggle();
    void onClearMacro();
    void onActionRecorded(const KeyboardMacroAction& action);

private:
    void setupUI();
    void setupModeSelector();
    void setupSingleKeyPage();
    void setupComboKeyPage();
    void setupHoldKeyPage();
    void setupTypeTextPage();
    void setupMacroPage();
    void setupLimitsGroup();
    void setupRandomGroup();
    void setupControlGroup();
    void connectSignals();

    // Вспомогательные
    void updateMacroTable();
    void setSettingsEnabled(bool enabled);

    // ==========================================
    // Движок
    // ==========================================
    KeyboardClicker* m_clicker         = nullptr;

    // ==========================================
    // Выбор режима
    // ==========================================
    QGroupBox*       m_modeGroup       = nullptr;
    QComboBox*       m_modeCombo       = nullptr;
    QStackedWidget*  m_modeStack       = nullptr;

    // ==========================================
    // Страница 1: Одиночная клавиша
    // ==========================================
    QWidget*         m_singleKeyPage   = nullptr;
    QPushButton*     m_captureKeyBtn   = nullptr;
    QLabel*          m_capturedKeyLabel = nullptr;
    QSpinBox*        m_intervalSpin    = nullptr;
    QCheckBox*       m_modCtrlCheck    = nullptr;
    QCheckBox*       m_modShiftCheck   = nullptr;
    QCheckBox*       m_modAltCheck     = nullptr;
    QCheckBox*       m_modWinCheck     = nullptr;
    int              m_capturedVkCode  = 0;
    bool             m_capturingKey    = false;

    // ==========================================
    // Страница 2: Комбинация клавиш
    // ==========================================
    QWidget*         m_comboKeyPage    = nullptr;
    QPushButton*     m_captureComboBtn = nullptr;
    QLabel*          m_capturedComboLabel = nullptr;
    QSpinBox*        m_comboIntervalSpin  = nullptr;
    QCheckBox*       m_comboCtrlCheck  = nullptr;
    QCheckBox*       m_comboShiftCheck = nullptr;
    QCheckBox*       m_comboAltCheck   = nullptr;
    QCheckBox*       m_comboWinCheck   = nullptr;
    int              m_comboVkCode     = 0;
    bool             m_capturingCombo  = false;

    // ==========================================
    // Страница 3: Зажатие клавиши (Hold)
    // ==========================================
    QWidget*         m_holdKeyPage     = nullptr;
    QPushButton*     m_captureHoldBtn  = nullptr;
    QLabel*          m_capturedHoldLabel = nullptr;
    QCheckBox*       m_holdCtrlCheck   = nullptr;
    QCheckBox*       m_holdShiftCheck  = nullptr;
    QCheckBox*       m_holdAltCheck    = nullptr;
    QCheckBox*       m_holdWinCheck    = nullptr;
    int              m_holdVkCode      = 0;
    bool             m_capturingHold   = false;

    // ==========================================
    // Страница 4: Ввод текста
    // ==========================================
    QWidget*         m_typeTextPage    = nullptr;
    QPlainTextEdit*  m_textEdit        = nullptr;
    QSpinBox*        m_typeDelaySpin   = nullptr;

    // ==========================================
    // Страница 4: Макрос клавиатуры
    // ==========================================
    QWidget*         m_macroPage       = nullptr;
    QTableWidget*    m_macroTable      = nullptr;
    QPushButton*     m_recordBtn       = nullptr;
    QPushButton*     m_clearMacroBtn   = nullptr;
    QLabel*          m_macroStatusLabel = nullptr;
    QSpinBox*        m_macroRepeatSpin = nullptr;

    // ==========================================
    // Ограничения
    // ==========================================
    QGroupBox*       m_limitsGroup     = nullptr;
    QCheckBox*       m_limitCountCheck = nullptr;
    QSpinBox*        m_maxCountSpin    = nullptr;
    QCheckBox*       m_limitTimeCheck  = nullptr;
    QSpinBox*        m_maxTimeSpin     = nullptr;

    // ==========================================
    // Рандомизация
    // ==========================================
    QGroupBox*       m_randomGroup     = nullptr;
    QCheckBox*       m_randomIntervalCheck = nullptr;
    QSpinBox*        m_randomMinSpin   = nullptr;
    QSpinBox*        m_randomMaxSpin   = nullptr;

    // ==========================================
    // Управление и статистика
    // ==========================================
    QGroupBox*       m_controlGroup    = nullptr;
    QPushButton*     m_startButton     = nullptr;
    QPushButton*     m_stopButton      = nullptr;
    QLabel*          m_statusLabel     = nullptr;
    QLabel*          m_pressCountLabel = nullptr;
    QLabel*          m_elapsedLabel    = nullptr;
};

