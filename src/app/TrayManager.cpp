// ===========================================
// TrayManager.cpp — Реализация системного трея
// ===========================================

#include "TrayManager.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QAction>
#include <QIcon>

TrayManager::TrayManager(QWidget* parentWindow)
    : QObject(parentWindow)
{
    setupTrayIcon(parentWindow);
    setupContextMenu(parentWindow);
    m_trayIcon->show();

    LOG_DEBUG(tr("Системный трей инициализирован"));
}

void TrayManager::setupTrayIcon(QWidget* parentWindow)
{
    m_trayIcon = new QSystemTrayIcon(parentWindow);
    m_trayIcon->setIcon(QIcon(":/icons/app.png"));
    m_trayIcon->setToolTip(tr("AutoClicker Suite"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &TrayManager::onTrayActivated);
}

void TrayManager::setupContextMenu(QWidget* parentWindow)
{
    m_trayMenu = new QMenu(parentWindow);

    // Действие "Показать"
    QAction* showAction = m_trayMenu->addAction(tr("Показать окно"));
    connect(showAction, &QAction::triggered, this, &TrayManager::showWindowRequested);

    m_trayMenu->addSeparator();

    // Горячие действия
    QAction* startStopAction = m_trayMenu->addAction(tr("▶ Старт/Стоп (F6)"));
    connect(startStopAction, &QAction::triggered, this, &TrayManager::startStopTriggered);

    QAction* recordAction = m_trayMenu->addAction(tr("⏺ Запись макроса (F7)"));
    connect(recordAction, &QAction::triggered, this, &TrayManager::recordMacroTriggered);

    QAction* emergencyAction = m_trayMenu->addAction(tr("⛔ Остановить всё (F8)"));
    connect(emergencyAction, &QAction::triggered, this, &TrayManager::emergencyStopTriggered);

    m_trayMenu->addSeparator();

    // Действие "Выход"
    QAction* quitAction = m_trayMenu->addAction(tr("Выход"));
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    m_trayIcon->setContextMenu(m_trayMenu);
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        emit showWindowRequested();
    }
}

void TrayManager::showMessage(const QString& title, const QString& message,
                               QSystemTrayIcon::MessageIcon icon, int timeoutMs)
{
    if (m_trayIcon) {
        m_trayIcon->showMessage(title, message, icon, timeoutMs);
    }
}

bool TrayManager::isVisible() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}
