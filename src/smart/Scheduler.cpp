// ===========================================
// Scheduler.cpp — Реализация планировщика макросов
// Проверяет задания каждую секунду, запускает
// макросы при наступлении запланированного времени
// ===========================================

#include "Scheduler.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>

Scheduler::Scheduler(QObject* parent)
    : QObject(parent)
{
    // Таймер проверки — каждую секунду
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(1000);
    connect(m_checkTimer, &QTimer::timeout, this, &Scheduler::onCheckTimer);

    // Загружаем задания с диска
    loadFromDisk();

    LOG_DEBUG(tr("Планировщик инициализирован, заданий: %1").arg(m_tasks.count()));
}

// ==========================================
// Управление заданиями
// ==========================================

void Scheduler::addTask(const SchedulerTask& task)
{
    m_tasks[task.id] = task;
    saveToDisk();
    emit taskListChanged();
    LOG_INFO(tr("Задание добавлено: %1 (макрос: %2, время: %3)")
        .arg(task.name, task.macroName,
             task.scheduledTime.toString("dd.MM.yyyy HH:mm:ss")));
}

void Scheduler::removeTask(const QString& taskId)
{
    if (m_tasks.contains(taskId)) {
        QString name = m_tasks[taskId].name;
        m_tasks.remove(taskId);
        saveToDisk();
        emit taskListChanged();
        LOG_INFO(tr("Задание удалено: %1").arg(name));
    }
}

void Scheduler::updateTask(const SchedulerTask& task)
{
    m_tasks[task.id] = task;
    saveToDisk();
    emit taskListChanged();
}

void Scheduler::setTaskEnabled(const QString& taskId, bool enabled)
{
    if (m_tasks.contains(taskId)) {
        m_tasks[taskId].enabled = enabled;
        saveToDisk();
        emit taskListChanged();
        LOG_INFO(tr("Задание '%1' %2")
            .arg(m_tasks[taskId].name,
                 enabled ? tr("включено") : tr("выключено")));
    }
}

SchedulerTask Scheduler::task(const QString& taskId) const
{
    return m_tasks.value(taskId);
}

QList<SchedulerTask> Scheduler::allTasks() const
{
    return m_tasks.values();
}

// ==========================================
// Управление планировщиком
// ==========================================

void Scheduler::start()
{
    if (m_running) return;
    m_running = true;
    m_checkTimer->start();
    LOG_INFO(tr("Планировщик запущен"));
}

void Scheduler::stop()
{
    if (!m_running) return;
    m_running = false;
    m_checkTimer->stop();
    LOG_INFO(tr("Планировщик остановлен"));
}

// ==========================================
// Проверка заданий
// ==========================================

void Scheduler::onCheckTimer()
{
    QDateTime now = QDateTime::currentDateTime();

    // Ищем ближайшее следующее задание (для уведомления)
    QDateTime nearestTime;
    QString   nearestName;

    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        SchedulerTask& task = it.value();
        if (!task.enabled) continue;

        // Проверяем, наступило ли время
        if (task.scheduledTime.isValid() && task.scheduledTime <= now) {
            // Запускаем задание!
            LOG_INFO(tr("Планировщик: запуск задания '%1' (макрос: %2)")
                .arg(task.name, task.macroName));

            emit taskTriggered(task.macroName);

            if (task.repeat) {
                // Вычисляем следующее время
                task.scheduledTime = now.addSecs(task.repeatIntervalMin * 60);
                LOG_INFO(tr("Следующий запуск '%1': %2")
                    .arg(task.name,
                         task.scheduledTime.toString("dd.MM.yyyy HH:mm:ss")));
            } else {
                // Одноразовое задание — выключаем
                task.enabled = false;
            }

            saveToDisk();
            emit taskListChanged();
        }

        // Ищем ближайшее срабатывание
        if (task.enabled && task.scheduledTime.isValid() && task.scheduledTime > now) {
            if (!nearestTime.isValid() || task.scheduledTime < nearestTime) {
                nearestTime = task.scheduledTime;
                nearestName = task.name;
            }
        }
    }

    if (nearestTime.isValid()) {
        emit nextTriggerInfo(nearestName, nearestTime);
    }
}

QDateTime Scheduler::nextTriggerTime(const SchedulerTask& task) const
{
    if (!task.enabled || !task.scheduledTime.isValid()) return {};

    QDateTime now = QDateTime::currentDateTime();
    if (task.scheduledTime > now) {
        return task.scheduledTime;
    }

    if (task.repeat) {
        // Вычисляем ближайшее будущее время
        qint64 intervalSec = task.repeatIntervalMin * 60;
        qint64 elapsed = task.scheduledTime.secsTo(now);
        qint64 periods = (elapsed / intervalSec) + 1;
        return task.scheduledTime.addSecs(periods * intervalSec);
    }

    return {};
}

// ==========================================
// Сериализация
// ==========================================

QJsonArray Scheduler::toJson() const
{
    QJsonArray arr;
    for (const auto& task : m_tasks) {
        arr.append(task.toJson());
    }
    return arr;
}

void Scheduler::fromJson(const QJsonArray& arr)
{
    m_tasks.clear();
    for (const auto& val : arr) {
        SchedulerTask task = SchedulerTask::fromJson(val.toObject());
        m_tasks[task.id] = task;
    }
}

void Scheduler::saveToDisk()
{
    QDir().mkpath(schedulerDir());
    QString path = schedulerDir() + "/scheduler.json";

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(QJsonObject{{"tasks", toJson()}});
        file.write(doc.toJson(QJsonDocument::Indented));
        LOG_DEBUG(tr("Задания планировщика сохранены"));
    }
}

void Scheduler::loadFromDisk()
{
    QString path = schedulerDir() + "/scheduler.json";
    QFile file(path);
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isNull()) {
            fromJson(doc.object()["tasks"].toArray());
            LOG_DEBUG(tr("Задания планировщика загружены: %1").arg(m_tasks.count()));
        }
    }
}

QString Scheduler::schedulerDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/" + AppConstants::Paths::PROFILES_DIR;
}

QString Scheduler::generateTaskId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

