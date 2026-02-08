#pragma once
// ===========================================
// TrayManager.h — Управление иконкой в системном трее
// ===========================================

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(QWidget* parentWindow);
    ~TrayManager() = default;

    // Показать всплывающее сообщение
    void showMessage(const QString& title, const QString& message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int timeoutMs = 3000);

    // Видима ли иконка в трее
    bool isVisible() const;

    // Перестроить меню при смене языка
    void retranslateUi();

signals:
    void showWindowRequested();
    void quitRequested();
    void startStopTriggered();
    void recordMacroTriggered();
    void emergencyStopTriggered();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupTrayIcon(QWidget* parentWindow);
    void setupContextMenu(QWidget* parentWindow);

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu*           m_trayMenu = nullptr;
};
