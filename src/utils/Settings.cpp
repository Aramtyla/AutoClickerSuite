// ===========================================
// Settings.cpp — Реализация обёртки над QSettings
// ===========================================

#include "Settings.h"
#include "Constants.h"

#include <QCoreApplication>
#include <QDir>

Settings& Settings::instance()
{
    static Settings settings;
    return settings;
}

Settings::Settings()
#ifdef PORTABLE_MODE
    // Портативный режим — настройки рядом с .exe
    : m_settings(QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini"),
                 QSettings::IniFormat)
#else
    // Стандартный режим — настройки в AppData
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 AppConstants::APP_ORGANIZATION,
                 AppConstants::APP_DISPLAY_NAME)
#endif
{
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
    emit settingChanged(key, value);
}

void Settings::remove(const QString& key)
{
    m_settings.remove(key);
}

bool Settings::contains(const QString& key) const
{
    return m_settings.contains(key);
}

int Settings::intValue(const QString& key, int defaultValue) const
{
    return m_settings.value(key, defaultValue).toInt();
}

bool Settings::boolValue(const QString& key, bool defaultValue) const
{
    return m_settings.value(key, defaultValue).toBool();
}

QString Settings::stringValue(const QString& key, const QString& defaultValue) const
{
    return m_settings.value(key, defaultValue).toString();
}

double Settings::doubleValue(const QString& key, double defaultValue) const
{
    return m_settings.value(key, defaultValue).toDouble();
}

void Settings::sync()
{
    m_settings.sync();
}

QString Settings::filePath() const
{
    return m_settings.fileName();
}
