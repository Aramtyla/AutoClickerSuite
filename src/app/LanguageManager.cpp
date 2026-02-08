// ===========================================
// LanguageManager.cpp — Реализация мультиязычности
// ===========================================

#include "LanguageManager.h"
#include "utils/Settings.h"
#include "utils/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

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

    // Для русского языка переводчик не нужен — исходные строки уже на русском
    if (lang == AppLanguage::Russian) {
        // Сохраняем выбор
        Settings::instance().setValue("appearance/language", "ru");
        emit languageChanged(lang);
        return;
    }

    // Определяем имя файла перевода
    QString baseName = "app_en";

    // Пробуем загрузить из ресурсов Qt (встроенные в бинарник)
    bool loaded = m_translator.load(":/translations/" + baseName + ".qm");

    // Фоллбэк — из файловой системы (рядом с exe)
    if (!loaded) {
        QString fsPath = QCoreApplication::applicationDirPath() + "/translations/" + baseName + ".qm";
        if (QFile::exists(fsPath)) {
            loaded = m_translator.load(fsPath);
        }
    }

    // Ещё один фоллбэк — из build-каталога (для отладки)
    if (!loaded) {
        loaded = m_translator.load(QCoreApplication::applicationDirPath() + "/" + baseName + ".qm");
    }

    // Фоллбэк — на уровень выше (если exe в подпапке Release/Debug)
    if (!loaded) {
        loaded = m_translator.load(QCoreApplication::applicationDirPath() + "/../" + baseName + ".qm");
    }

    if (loaded) {
        QCoreApplication::installTranslator(&m_translator);
        LOG_DEBUG(tr("Файл перевода загружен: %1").arg(baseName));
    } else {
        LOG_WARNING(tr("Файл перевода не найден: %1 (используются встроенные строки)").arg(baseName));
    }

    // Сохраняем выбор
    Settings::instance().setValue("appearance/language", "en");

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
