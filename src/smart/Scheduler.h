#pragma once
// ===========================================
// Scheduler.h — Планировщик макросов
// Запуск макросов по расписанию с повторением
// ===========================================

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>

#include "SmartConfig.h"

class Scheduler : public QObject
{
    Q_OBJECT

public:
    explicit Scheduler(QObject* parent = nullptr);

    // ==========================================
    // Управление заданиями
    // ==========================================

    // Добавить задание
    void addTask(const SchedulerTask& task);

    // Удалить задание
    void removeTask(const QString& taskId);

    // Обновить задание
    void updateTask(const SchedulerTask& task);

    // Включить/выключить задание
    void setTaskEnabled(const QString& taskId, bool enabled);

    // Получить задание по ID
    SchedulerTask task(const QString& taskId) const;

    // Получить все задания
    QList<SchedulerTask> allTasks() const;

    // Количество заданий
    int taskCount() const { return m_tasks.count(); }

    // ==========================================
    // Управление планировщиком
    // ==========================================

    // Запустить/остановить планировщик
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // ==========================================
    // Сериализация
    // ==========================================
    QJsonArray toJson() const;
    void fromJson(const QJsonArray& arr);

    // Сохранить/загрузить на диск
    void saveToDisk();
    void loadFromDisk();

    // Сгенерировать уникальный ID задания
    static QString generateTaskId();

signals:
    // Пора выполнить задание
    void taskTriggered(const QString& macroName);

    // Задание добавлено/удалено/изменено
    void taskListChanged();

    // Следующее срабатывание
    void nextTriggerInfo(const QString& taskName, const QDateTime& time);

private slots:
    void onCheckTimer();

private:
    // Вычислить следующее время срабатывания задания
    QDateTime nextTriggerTime(const SchedulerTask& task) const;

    // Каталог хранения
    QString schedulerDir() const;

    QMap<QString, SchedulerTask> m_tasks;
    QTimer*  m_checkTimer = nullptr;
    bool     m_running    = false;
};

