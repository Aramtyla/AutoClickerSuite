#pragma once
// ===========================================
// SettingsWidget.h — Диалог настроек приложения
// Настройка горячих клавиш, общие параметры,
// внешний вид и информация о программе
// ===========================================

#include <QDialog>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QKeySequenceEdit>
#include <QSpinBox>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class HotkeyManager;
class ThemeManager;
class LanguageManager;

class SettingsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWidget(HotkeyManager* hotkeyMgr,
                            ThemeManager* themeMgr,
                            LanguageManager* langMgr,
                            QWidget* parent = nullptr);

signals:
    void settingsApplied();
    void hotkeysChanged();
    void restartRequired();

private slots:
    void onApply();
    void onResetHotkeys();
    void onCaptureHotkey(int hotkeyIndex);

private:
    void setupUI();
    void setupHotkeyTab(QWidget* tab);
    void setupGeneralTab(QWidget* tab);
    void setupAboutTab(QWidget* tab);
    void loadSettings();
    void saveSettings();

    // Вспомогательные
    QString hotkeyToString(UINT modifiers, UINT vk) const;
    void updateHotkeyLabel(int index);

    // Менеджеры
    HotkeyManager*   m_hotkeyMgr  = nullptr;
    ThemeManager*    m_themeMgr   = nullptr;
    LanguageManager* m_langMgr    = nullptr;

    QTabWidget* m_tabs = nullptr;

    // === Горячие клавиши ===
    struct HotkeyBinding {
        int     id;
        QString name;
        UINT    modifiers;
        UINT    vk;
        QPushButton* captureBtn = nullptr;
        QLabel*      keyLabel   = nullptr;
    };
    QList<HotkeyBinding> m_hotkeyBindings;
    int m_capturingIndex = -1;  // Какой хоткей сейчас захватываем (-1 = нет)

    // === Общие настройки ===
    QCheckBox*  m_minimizeToTray   = nullptr;
    QCheckBox*  m_startMinimized   = nullptr;
    QCheckBox*  m_confirmExit      = nullptr;
    QComboBox*  m_themeCombo       = nullptr;
    QComboBox*  m_langCombo        = nullptr;
    QSpinBox*   m_logMaxLinesSpin  = nullptr;
    QCheckBox*  m_logToFileCb      = nullptr;

    // Кнопки
    QPushButton* m_applyBtn  = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QPushButton* m_okBtn     = nullptr;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};
