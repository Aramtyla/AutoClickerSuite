#pragma once
// ===========================================
// ThemeManager.h — Управление темами (тёмная / светлая)
// ===========================================

#include <QObject>
#include <QString>

enum class AppTheme {
    Dark,
    Light
};

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    explicit ThemeManager(QObject* parent = nullptr);

    // Применить тему
    void applyTheme(AppTheme theme);

    // Переключить тему (dark <-> light)
    void toggleTheme();

    // Текущая тема
    AppTheme currentTheme() const;
    QString currentThemeName() const;

    // Загрузить тему из настроек (при старте)
    void loadFromSettings();

signals:
    void themeChanged(AppTheme theme);

private:
    // Загрузить QSS-стиль из файла ресурсов
    QString loadStyleSheet(const QString& path) const;

    AppTheme m_currentTheme = AppTheme::Dark;
};
