#pragma once
// ===========================================
// Logger.h — Система логирования в реальном времени
// ===========================================

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMutex>

// Уровни логирования
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger : public QObject
{
    Q_OBJECT

public:
    // Singleton — единственный экземпляр логгера
    static Logger& instance();

    // Методы логирования
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

    // Общий метод
    void log(LogLevel level, const QString& message);

    // Получить все записи
    QStringList entries() const;

    // Очистить лог
    void clear();

    // Включение/выключение записи в файл
    void setFileLoggingEnabled(bool enabled);
    bool isFileLoggingEnabled() const;

signals:
    // Сигнал для обновления GUI лога в реальном времени
    void logAdded(const QString& formattedMessage, LogLevel level);

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Форматирование сообщения
    QString formatMessage(LogLevel level, const QString& message) const;
    QString levelToString(LogLevel level) const;

    // Запись в файл
    void writeToFile(const QString& formattedMessage);

    QStringList m_entries;        // Все записи
    mutable QMutex m_mutex;       // Потокобезопасность
    bool m_fileLogging = false;   // Логирование в файл
    int m_maxEntries = 5000;      // Максимум записей
};

// Удобные макросы для логирования
#define LOG_DEBUG(msg)   Logger::instance().debug(msg)
#define LOG_INFO(msg)    Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg)   Logger::instance().error(msg)
