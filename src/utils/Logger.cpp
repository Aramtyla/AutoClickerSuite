// ===========================================
// Logger.cpp — Реализация системы логирования
// ===========================================

#include "Logger.h"
#include "Constants.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QMutexLocker>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
    : m_maxEntries(AppConstants::UI::LOG_MAX_LINES)
{
}

void Logger::debug(const QString& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const QString& message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(const QString& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const QString& message)
{
    log(LogLevel::Error, message);
}

void Logger::log(LogLevel level, const QString& message)
{
    QString formatted = formatMessage(level, message);

    {
        QMutexLocker locker(&m_mutex);

        // Ограничиваем размер лога
        if (m_entries.size() >= m_maxEntries) {
            m_entries.removeFirst();
        }
        m_entries.append(formatted);
    }

    // Запись в файл, если включено
    if (m_fileLogging) {
        writeToFile(formatted);
    }

    // Отправляем сигнал для обновления GUI
    emit logAdded(formatted, level);
}

QStringList Logger::entries() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries;
}

void Logger::clear()
{
    QMutexLocker locker(&m_mutex);
    m_entries.clear();
}

void Logger::setFileLoggingEnabled(bool enabled)
{
    m_fileLogging = enabled;
}

bool Logger::isFileLoggingEnabled() const
{
    return m_fileLogging;
}

QString Logger::formatMessage(LogLevel level, const QString& message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    return QString("[%1] [%2] %3")
        .arg(timestamp)
        .arg(levelToString(level))
        .arg(message);
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::writeToFile(const QString& formattedMessage)
{
    // Путь к файлу лога: <AppData>/AutoClickerSuite/logs/
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + "/" + AppConstants::Paths::LOGS_DIR;
    QDir().mkpath(logDir);

    QString logPath = logDir + "/autoclicker.log";
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << formattedMessage << "\n";
    }
}
