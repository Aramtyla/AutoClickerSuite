#pragma once
// ===========================================
// LanguageManager.h — Мультиязычность (RU / EN)
// ===========================================

#include <QObject>
#include <QTranslator>
#include <QString>

enum class AppLanguage {
    Russian,
    English
};

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    explicit LanguageManager(QObject* parent = nullptr);

    // Установить язык
    void setLanguage(AppLanguage lang);

    // Переключить язык (RU <-> EN)
    void toggleLanguage();

    // Текущий язык
    AppLanguage currentLanguage() const;
    QString currentLanguageName() const;

    // Загрузить язык из настроек
    void loadFromSettings();

signals:
    void languageChanged(AppLanguage lang);

private:
    QTranslator  m_translator;
    AppLanguage  m_currentLanguage = AppLanguage::Russian;
};
