#pragma once
// ===========================================
// ProfileManager.h — Управление профилями настроек
// Полная реализация: сбор и применение настроек
// из всех модулей (мышь, клавиатура, умные режимы)
// ===========================================

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QStringList>

// Предварительные объявления
class MouseWidget;
class KeyboardWidget;
class SmartWidget;

class ProfileManager : public QObject
{
    Q_OBJECT

public:
    explicit ProfileManager(QObject* parent = nullptr);

    // Установить ссылки на виджеты модулей для сбора/применения настроек
    void setModuleWidgets(MouseWidget* mouse, KeyboardWidget* keyboard,
                          SmartWidget* smart);

    // Сохранить текущие настройки в профиль
    bool saveProfile(const QString& name);

    // Загрузить профиль
    bool loadProfile(const QString& name);

    // Удалить профиль
    bool deleteProfile(const QString& name);

    // Список доступных профилей
    QStringList availableProfiles() const;

    // Текущий активный профиль
    QString currentProfile() const;

    // Путь к директории профилей
    QString profilesDirectory() const;

signals:
    void profileLoaded(const QString& name);
    void profileSaved(const QString& name);
    void profileListChanged();

private:
    // Собрать текущие настройки в JSON
    QJsonObject collectSettings() const;

    // Применить настройки из JSON
    void applySettings(const QJsonObject& settings);

    QString m_currentProfile;

    // Ссылки на виджеты модулей
    MouseWidget*    m_mouseWidget    = nullptr;
    KeyboardWidget* m_keyboardWidget = nullptr;
    SmartWidget*    m_smartWidget    = nullptr;
};
