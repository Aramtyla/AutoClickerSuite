#pragma once
// ===========================================
// Settings.h — Обёртка над QSettings для хранения настроек
// ===========================================

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QString>

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings& instance();

    // Общие методы чтения/записи
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    void remove(const QString& key);
    bool contains(const QString& key) const;

    // Удобные типизированные геттеры
    int intValue(const QString& key, int defaultValue = 0) const;
    bool boolValue(const QString& key, bool defaultValue = false) const;
    QString stringValue(const QString& key, const QString& defaultValue = QString()) const;
    double doubleValue(const QString& key, double defaultValue = 0.0) const;

    // Синхронизация с диском
    void sync();

    // Путь к файлу настроек
    QString filePath() const;

signals:
    void settingChanged(const QString& key, const QVariant& value);

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    QSettings m_settings;
};
