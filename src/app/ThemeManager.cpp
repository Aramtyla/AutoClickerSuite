// ===========================================
// ThemeManager.cpp — Реализация переключения тем
// ===========================================

#include "ThemeManager.h"
#include "utils/Settings.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    loadFromSettings();
}

void ThemeManager::applyTheme(AppTheme theme)
{
    m_currentTheme = theme;

    QString qssPath;
    switch (theme) {
        case AppTheme::Dark:
            qssPath = ":/themes/dark.qss";
            break;
        case AppTheme::Light:
            qssPath = ":/themes/light.qss";
            break;
    }

    QString style = loadStyleSheet(qssPath);
    qApp->setStyleSheet(style);

    // Сохраняем выбор
    Settings::instance().setValue("appearance/theme",
                                  theme == AppTheme::Dark ? "dark" : "light");

    emit themeChanged(theme);
}

void ThemeManager::toggleTheme()
{
    if (m_currentTheme == AppTheme::Dark) {
        applyTheme(AppTheme::Light);
    } else {
        applyTheme(AppTheme::Dark);
    }
}

AppTheme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::currentThemeName() const
{
    return m_currentTheme == AppTheme::Dark ? tr("Тёмная") : tr("Светлая");
}

void ThemeManager::loadFromSettings()
{
    QString saved = Settings::instance().stringValue("appearance/theme", "dark");
    AppTheme theme = (saved == "light") ? AppTheme::Light : AppTheme::Dark;
    applyTheme(theme);
}

QString ThemeManager::loadStyleSheet(const QString& path) const
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }

    LOG_WARNING(tr("Не удалось загрузить стиль: %1").arg(path));
    return QString();
}
