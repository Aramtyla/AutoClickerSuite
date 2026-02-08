// ===========================================
// MouseClicker.cpp — Реализация движка автоклика
// ===========================================

#include "MouseClicker.h"
#include "core/InputSimulator.h"
#include "utils/Logger.h"

#include <QThread>
#include <QtMath>

MouseClicker::MouseClicker(QObject* parent)
    : QObject(parent)
{
    m_input = new InputSimulator(this);

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);  // Высокая точность для малых интервалов
    connect(m_timer, &QTimer::timeout, this, &MouseClicker::onTimerTick);
}

MouseClicker::~MouseClicker()
{
    stop();
}

// ===========================================
// Управление
// ===========================================

void MouseClicker::start()
{
    if (m_running) return;

    m_running    = true;
    m_clickCount = 0;
    m_currentPointIndex = 0;
    m_holdActive = false;
    m_elapsed.start();

    // Запускаем таймер с первым интервалом
    m_timer->start(nextInterval());

    LOG_INFO(tr("Автоклик мыши запущен (интервал: %1 мс, кнопка: %2)")
                 .arg(m_config.intervalMs)
                 .arg(m_config.button == MouseButton::Left   ? "ЛКМ" :
                      m_config.button == MouseButton::Right  ? "ПКМ" : "СКМ"));

    emit started();
}

void MouseClicker::stop()
{
    if (!m_running) return;

    m_timer->stop();
    m_running = false;

    // Если было удержание — отпускаем кнопку
    if (m_holdActive) {
        int btn = static_cast<int>(m_config.button);
        m_input->mouseUp(btn);
        m_holdActive = false;
    }

    qint64 elapsed = m_elapsed.elapsed();
    LOG_INFO(tr("Автоклик мыши остановлен (кликов: %1, время: %2 мс)")
                 .arg(m_clickCount).arg(elapsed));

    emit statsUpdated(m_clickCount, elapsed);
    emit stopped();
}

void MouseClicker::toggleStartStop()
{
    if (m_running) {
        stop();
    } else {
        start();
    }
}

bool MouseClicker::isRunning() const
{
    return m_running;
}

// ===========================================
// Конфигурация
// ===========================================

void MouseClicker::setConfig(const MouseClickerConfig& config)
{
    m_config = config;

    // Если таймер активен — обновляем интервал
    if (m_running) {
        m_timer->setInterval(nextInterval());
    }
}

MouseClickerConfig MouseClicker::config() const
{
    return m_config;
}

// ===========================================
// Мультиточечный режим
// ===========================================

void MouseClicker::setClickPoints(const QVector<QPoint>& points)
{
    m_clickPoints = points;
}

QVector<QPoint> MouseClicker::clickPoints() const
{
    return m_clickPoints;
}

void MouseClicker::addClickPoint(const QPoint& point)
{
    m_clickPoints.append(point);
}

void MouseClicker::removeClickPoint(int index)
{
    if (index >= 0 && index < m_clickPoints.size()) {
        m_clickPoints.removeAt(index);
    }
}

void MouseClicker::clearClickPoints()
{
    m_clickPoints.clear();
}

// ===========================================
// Статистика
// ===========================================

qint64 MouseClicker::totalClicks() const
{
    return m_clickCount;
}

qint64 MouseClicker::elapsedMs() const
{
    if (m_running) {
        return m_elapsed.elapsed();
    }
    return 0;
}

// ===========================================
// Основной цикл
// ===========================================

void MouseClicker::onTimerTick()
{
    if (!m_running) return;

    // Проверяем лимиты перед кликом
    if (!checkLimits()) {
        stop();
        emit finished();
        return;
    }

    // Выполняем клик
    performClick();

    // Обновляем интервал (для рандомизации)
    if (m_config.randomizeInterval) {
        m_timer->setInterval(nextInterval());
    }

    // Обновляем статистику каждые 10 кликов (чтобы не нагружать GUI)
    if (m_clickCount % 10 == 0) {
        emit statsUpdated(m_clickCount, m_elapsed.elapsed());
    }
}

void MouseClicker::performClick()
{
    int btn = static_cast<int>(m_config.button);

    // ==========================================
    // Вычисляем позицию клика
    // ==========================================
    QPoint pos = calculateClickPosition();

    // Перемещаем курсор, если не режим FollowCursor
    if (m_config.positionMode != ClickPosition::FollowCursor) {
        m_input->mouseMoveTo(pos.x(), pos.y());
        // Небольшая пауза для стабильности перемещения
        QThread::usleep(500);
    }

    // ==========================================
    // Выполняем клик по типу
    // ==========================================
    switch (m_config.clickType) {
        case ClickType::Single:
            m_input->mouseClick(btn);
            break;

        case ClickType::Double:
            m_input->mouseDoubleClick(btn);
            break;

        case ClickType::Hold:
            if (!m_holdActive) {
                // Первый тик — нажимаем
                m_input->mouseDown(btn);
                m_holdActive = true;
            } else {
                // Второй тик — отпускаем
                m_input->mouseUp(btn);
                m_holdActive = false;
            }
            break;
    }

    m_clickCount++;

    // Получаем текущую позицию для сигнала
    QPoint actualPos = m_input->cursorPosition();
    emit clicked(static_cast<int>(m_clickCount), actualPos);

    // Для мультиточечного режима — переходим к следующей точке
    if (m_config.positionMode == ClickPosition::MultiPoint && !m_clickPoints.isEmpty()) {
        m_currentPointIndex = (m_currentPointIndex + 1) % m_clickPoints.size();
    }
}

// ===========================================
// Вычисление интервала
// ===========================================

int MouseClicker::nextInterval() const
{
    if (m_config.randomizeInterval) {
        int minI = m_config.randomIntervalMin;
        int maxI = m_config.randomIntervalMax;
        return QRandomGenerator::global()->bounded(minI, maxI + 1);
    }
    return m_config.intervalMs;
}

// ===========================================
// Вычисление позиции
// ===========================================

QPoint MouseClicker::calculateClickPosition()
{
    QPoint pos;

    switch (m_config.positionMode) {
        case ClickPosition::FollowCursor:
            pos = m_input->cursorPosition();
            break;

        case ClickPosition::FixedPosition:
            pos = m_config.fixedPos;
            break;

        case ClickPosition::MultiPoint:
            if (!m_clickPoints.isEmpty()) {
                pos = m_clickPoints[m_currentPointIndex];
            } else {
                pos = m_input->cursorPosition();
            }
            break;
    }

    // Рандомизация позиции
    if (m_config.randomizePosition) {
        int offset = m_config.randomOffsetPixels;
        int dx = QRandomGenerator::global()->bounded(-offset, offset + 1);
        int dy = QRandomGenerator::global()->bounded(-offset, offset + 1);
        pos += QPoint(dx, dy);
    }

    return pos;
}

// ===========================================
// Проверка лимитов
// ===========================================

bool MouseClicker::checkLimits()
{
    // Лимит по количеству кликов
    if (m_config.clickCount > 0 && m_clickCount >= m_config.clickCount) {
        LOG_INFO(tr("Достигнут лимит кликов: %1").arg(m_config.clickCount));
        return false;
    }

    // Лимит по времени
    if (m_config.timeLimitMs > 0 && m_elapsed.elapsed() >= m_config.timeLimitMs) {
        LOG_INFO(tr("Достигнут лимит времени: %1 мс").arg(m_config.timeLimitMs));
        return false;
    }

    return true;
}
