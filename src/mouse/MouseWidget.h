#pragma once
// ===========================================
// MouseWidget.h — Полный GUI модуля автоклика мыши
// ===========================================

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QTimer>

class MouseClicker;

class MouseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MouseWidget(QWidget* parent = nullptr);
    ~MouseWidget();

    // Внешний доступ для MainWindow
    MouseClicker* clicker() const { return m_clicker; }

public slots:
    void toggleStartStop();

private slots:
    void onStartClicked();
    void onStopClicked();
    void applyConfig();
    void onStatsUpdated(qint64 clicks, qint64 elapsedMs);
    void onClickerStarted();
    void onClickerStopped();
    void onClickerFinished();
    void onPickPosition();
    void onAddPoint();
    void onRemovePoint();
    void onClearPoints();
    void updatePositionLabel();

private:
    void setupUI();
    void setupClickSettingsGroup();
    void setupPositionGroup();
    void setupLimitsGroup();
    void setupRandomGroup();
    void setupControlGroup();
    void setupMultiPointGroup();
    void connectSignals();

    // Движок
    MouseClicker*   m_clicker           = nullptr;

    // Настройки клика
    QGroupBox*      m_clickGroup        = nullptr;
    QComboBox*      m_buttonCombo       = nullptr;
    QComboBox*      m_clickTypeCombo    = nullptr;
    QSpinBox*       m_intervalSpin      = nullptr;
    QSpinBox*       m_holdDurationSpin  = nullptr;

    // Позиция
    QGroupBox*      m_posGroup          = nullptr;
    QRadioButton*   m_followCursorRadio = nullptr;
    QRadioButton*   m_fixedPosRadio     = nullptr;
    QRadioButton*   m_multiPointRadio   = nullptr;
    QSpinBox*       m_fixedXSpin        = nullptr;
    QSpinBox*       m_fixedYSpin        = nullptr;
    QPushButton*    m_pickPosButton     = nullptr;
    QLabel*         m_currentPosLabel   = nullptr;

    // Мультиточечный режим
    QGroupBox*      m_multiGroup        = nullptr;
    QTableWidget*   m_pointsTable       = nullptr;
    QPushButton*    m_addPointBtn       = nullptr;
    QPushButton*    m_removePointBtn    = nullptr;
    QPushButton*    m_clearPointsBtn    = nullptr;

    // Ограничения
    QGroupBox*      m_limitsGroup       = nullptr;
    QCheckBox*      m_limitClicksCheck  = nullptr;
    QSpinBox*       m_maxClicksSpin     = nullptr;
    QCheckBox*      m_limitTimeCheck    = nullptr;
    QSpinBox*       m_maxTimeSpin       = nullptr;

    // Рандомизация
    QGroupBox*      m_randomGroup       = nullptr;
    QCheckBox*      m_randomIntervalCheck = nullptr;
    QSpinBox*       m_randomMinSpin     = nullptr;
    QSpinBox*       m_randomMaxSpin     = nullptr;
    QCheckBox*      m_randomPosCheck    = nullptr;
    QSpinBox*       m_randomOffsetSpin  = nullptr;

    // Управление и статистика
    QGroupBox*      m_controlGroup      = nullptr;
    QPushButton*    m_startButton       = nullptr;
    QPushButton*    m_stopButton        = nullptr;
    QLabel*         m_statusLabel       = nullptr;
    QLabel*         m_clickCountLabel   = nullptr;
    QLabel*         m_elapsedLabel      = nullptr;

    // Таймер обновления позиции курсора
    QTimer*         m_posUpdateTimer    = nullptr;

    // Флаг режима захвата координат
    bool            m_pickingPosition   = false;
};
