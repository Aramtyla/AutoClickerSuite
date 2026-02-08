#pragma once
// ===========================================
// MouseClicker.h — Движок автоклика мыши
// Работает в отдельном QTimer, потокобезопасен
// ===========================================

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QPoint>
#include <QRandomGenerator>

#include "MouseConfig.h"

class InputSimulator;

class MouseClicker : public QObject
{
    Q_OBJECT

public:
    explicit MouseClicker(QObject* parent = nullptr);
    ~MouseClicker();

    // ==========================================
    // Управление
    // ==========================================
    void start();           // Запустить автоклик
    void stop();            // Остановить автоклик
    void toggleStartStop(); // Переключить старт/стоп
    bool isRunning() const;

    // ==========================================
    // Конфигурация
    // ==========================================
    void setConfig(const MouseClickerConfig& config);
    MouseClickerConfig config() const;

    // ==========================================
    // Мультиточечный режим
    // ==========================================
    void setClickPoints(const QVector<QPoint>& points);
    QVector<QPoint> clickPoints() const;
    void addClickPoint(const QPoint& point);
    void removeClickPoint(int index);
    void clearClickPoints();

    // Статистика
    qint64 totalClicks() const;
    qint64 elapsedMs() const;

signals:
    void started();
    void stopped();
    void clicked(int clickNumber, const QPoint& position);
    void finished();        // Завершение по лимиту кликов/времени
    void error(const QString& message);
    void statsUpdated(qint64 clicks, qint64 elapsedMs);

private slots:
    void onTimerTick();

private:
    // Выполнить один клик
    void performClick();

    // Рассчитать следующий интервал (с учётом рандомизации)
    int nextInterval() const;

    // Рассчитать позицию клика (с учётом режима и рандомизации)
    QPoint calculateClickPosition();

    // Проверить лимиты (кликов / времени)
    bool checkLimits();

    // Состояние
    InputSimulator*     m_input       = nullptr;
    QTimer*             m_timer       = nullptr;
    QElapsedTimer       m_elapsed;
    MouseClickerConfig  m_config;
    bool                m_running     = false;

    // Статистика
    qint64              m_clickCount  = 0;

    // Мультиточечный режим
    QVector<QPoint>     m_clickPoints;
    int                 m_currentPointIndex = 0;

    // Для режима Hold — отслеживание нажатия
    bool                m_holdActive  = false;
};
