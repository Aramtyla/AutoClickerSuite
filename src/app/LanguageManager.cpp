// ===========================================
// LanguageManager.cpp — Реализация мультиязычности
// ===========================================

#include "LanguageManager.h"
#include "utils/Settings.h"
#include "utils/Logger.h"

#include <QCoreApplication>
#include <QDir>

LanguageManager::LanguageManager(QObject* parent)
    : QObject(parent)
{
    loadFromSettings();
}

void LanguageManager::setLanguage(AppLanguage lang)
{
    m_currentLanguage = lang;

    // Удаляем старый переводчик
    QCoreApplication::removeTranslator(&m_translator);

    // Определяем имя файла перевода
    QString baseName;
    switch (lang) {
        case AppLanguage::Russian:
            baseName = "app_ru";
            break;
        case AppLanguage::English:
            baseName = "app_en";
            break;
    }

    // Пробуем загрузить из файловой системы (рядом с exe)
    QString fsPath = QCoreApplication::applicationDirPath() + "/translations/" + baseName + ".qm";
    bool loaded = false;

    if (QFile::exists(fsPath)) {
        loaded = m_translator.load(fsPath);
    }

    // Фоллбэк — из ресурсов Qt
    if (!loaded) {
        loaded = m_translator.load(":/translations/" + baseName + ".qm");
    }

    if (loaded) {
        QCoreApplication::installTranslator(&m_translator);
        LOG_DEBUG(tr("Файл перевода загружен: %1").arg(baseName));
    } else {
        LOG_DEBUG(tr("Файл перевода не найден: %1 (используются встроенные строки)")
                      .arg(baseName));
    }

    // Сохраняем выбор
    Settings::instance().setValue("appearance/language",
                                  lang == AppLanguage::Russian ? "ru" : "en");

    emit languageChanged(lang);
}

void LanguageManager::toggleLanguage()
{
    if (m_currentLanguage == AppLanguage::Russian) {
        setLanguage(AppLanguage::English);
    } else {
        setLanguage(AppLanguage::Russian);
    }
}

AppLanguage LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

QString LanguageManager::currentLanguageName() const
{
    return m_currentLanguage == AppLanguage::Russian ? "Русский" : "English";
}

void LanguageManager::loadFromSettings()
{
    QString saved = Settings::instance().stringValue("appearance/language", "ru");
    AppLanguage lang = (saved == "en") ? AppLanguage::English : AppLanguage::Russian;
    setLanguage(lang);
}
