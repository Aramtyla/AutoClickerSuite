#pragma once
// ===========================================
// SmartWidget.h — Полный GUI вкладки умных режимов
// Привязка к окну, клик по цвету, клик по
// изображению, планировщик макросов, профили
// ===========================================

#include <QWidget>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QListWidget>
#include <QSlider>
#include <QColor>

class WindowFinder;
class ColorMatcher;
class ImageMatcher;
class Scheduler;

class SmartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SmartWidget(QWidget* parent = nullptr);
    ~SmartWidget();

    // Внешний доступ для MainWindow
    WindowFinder* windowFinder() const { return m_windowFinder; }
    ColorMatcher* colorMatcher() const { return m_colorMatcher; }
    ImageMatcher* imageMatcher() const { return m_imageMatcher; }
    Scheduler*    scheduler()    const { return m_scheduler; }

    // Экстренная остановка всех умных режимов
    void emergencyStop();

    // Сбор/применение настроек для профилей
    QJsonObject collectSettings() const;
    void applySettings(const QJsonObject& settings);

    // Получить список макросов (устанавливается из MainWindow)
    void setAvailableMacros(const QStringList& macros);

signals:
    // Запрос на запуск макроса (для планировщика)
    void runMacroRequested(const QString& macroName);

private slots:
    // === Привязка к окну ===
    void onPickWindow();
    void onWindowPicked(quintptr hwnd, const QString& title);
    void onRefreshWindows();
    void onWindowSelected(int index);
    void onBindingToggled(bool enabled);

    // === Клик по цвету ===
    void onPickColor();
    void onColorStart();
    void onColorStop();
    void onColorFound(const QPoint& pos);

    // === Клик по изображению ===
    void onLoadTemplate();
    void onImageStart();
    void onImageStop();
    void onImageFound(const QPoint& pos, double score);

    // === Планировщик ===
    void onAddSchedulerTask();
    void onRemoveSchedulerTask();
    void onSchedulerToggle();
    void onTaskTriggered(const QString& macroName);
    void onSchedulerTaskListChanged();

private:
    void setupUI();

    // Подвкладки
    void setupWindowBindingTab(QWidget* tab);
    void setupColorMatchTab(QWidget* tab);
    void setupImageMatchTab(QWidget* tab);
    void setupSchedulerTab(QWidget* tab);
    void setupProfilesTab(QWidget* tab);

    void connectSignals();

    // ==========================================
    // Движки
    // ==========================================
    WindowFinder*  m_windowFinder  = nullptr;
    ColorMatcher*  m_colorMatcher  = nullptr;
    ImageMatcher*  m_imageMatcher  = nullptr;
    Scheduler*     m_scheduler     = nullptr;

    // ==========================================
    // Подвкладки
    // ==========================================
    QTabWidget*    m_subTabs       = nullptr;

    // === Привязка к окну ===
    QGroupBox*     m_bindGroup     = nullptr;
    QCheckBox*     m_bindEnable    = nullptr;
    QComboBox*     m_windowCombo   = nullptr;
    QPushButton*   m_pickWindowBtn = nullptr;
    QPushButton*   m_refreshBtn    = nullptr;
    QLabel*        m_windowInfoLabel = nullptr;
    QCheckBox*     m_bringToFrontCheck = nullptr;
    QCheckBox*     m_useClientAreaCheck = nullptr;
    QLabel*        m_windowPreviewLabel = nullptr;

    // === Клик по цвету ===
    QGroupBox*     m_colorGroup    = nullptr;
    QPushButton*   m_pickColorBtn  = nullptr;
    QLabel*        m_colorPreview  = nullptr;
    QSpinBox*      m_colorRSpin    = nullptr;
    QSpinBox*      m_colorGSpin    = nullptr;
    QSpinBox*      m_colorBSpin    = nullptr;
    QSpinBox*      m_colorTolerance = nullptr;
    QSpinBox*      m_colorInterval = nullptr;
    QComboBox*     m_colorActionCombo = nullptr;
    QCheckBox*     m_colorWaitDisappear = nullptr;
    QCheckBox*     m_colorFullScreen = nullptr;
    QSpinBox*      m_colorAreaX    = nullptr;
    QSpinBox*      m_colorAreaY    = nullptr;
    QSpinBox*      m_colorAreaW    = nullptr;
    QSpinBox*      m_colorAreaH    = nullptr;
    QPushButton*   m_colorStartBtn = nullptr;
    QPushButton*   m_colorStopBtn  = nullptr;
    QLabel*        m_colorStatusLabel = nullptr;

    // === Клик по изображению ===
    QGroupBox*     m_imageGroup    = nullptr;
    QPushButton*   m_loadTemplateBtn = nullptr;
    QLabel*        m_templatePreview = nullptr;
    QLabel*        m_templateInfoLabel = nullptr;
    QDoubleSpinBox* m_imageThreshold = nullptr;
    QSpinBox*      m_imageInterval = nullptr;
    QComboBox*     m_imageActionCombo = nullptr;
    QCheckBox*     m_imageWaitDisappear = nullptr;
    QCheckBox*     m_imageFullScreen = nullptr;
    QCheckBox*     m_imageGrayscale = nullptr;
    QPushButton*   m_imageStartBtn = nullptr;
    QPushButton*   m_imageStopBtn  = nullptr;
    QLabel*        m_imageStatusLabel = nullptr;

    // === Планировщик ===
    QGroupBox*     m_schedGroup    = nullptr;
    QTableWidget*  m_schedTable    = nullptr;
    QLineEdit*     m_schedNameEdit = nullptr;
    QComboBox*     m_schedMacroCombo = nullptr;
    QDateTimeEdit* m_schedDateTime = nullptr;
    QCheckBox*     m_schedRepeatCheck = nullptr;
    QSpinBox*      m_schedRepeatInterval = nullptr;
    QPushButton*   m_schedAddBtn   = nullptr;
    QPushButton*   m_schedRemoveBtn = nullptr;
    QPushButton*   m_schedToggleBtn = nullptr;
    QLabel*        m_schedStatusLabel = nullptr;

    // === Профили ===
    QGroupBox*     m_profileGroup  = nullptr;
    QListWidget*   m_profileList   = nullptr;
    QLineEdit*     m_profileNameEdit = nullptr;
    QPushButton*   m_profileSaveBtn = nullptr;
    QPushButton*   m_profileLoadBtn = nullptr;
    QPushButton*   m_profileDeleteBtn = nullptr;

    // Данные окон для ComboBox
    QList<quintptr> m_windowHandles;

    // Текущий цвет
    QColor         m_selectedColor = Qt::red;
};
