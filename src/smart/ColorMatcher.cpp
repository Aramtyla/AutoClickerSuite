// ===========================================
// ColorMatcher.cpp — Реализация поиска цвета на экране
// Сканирует область экрана/окна, ищет пиксель
// заданного цвета с допуском. Использует Win32 GDI.
// ===========================================

#include "ColorMatcher.h"
#include "WindowFinder.h"
#include "utils/Logger.h"
#include "core/InputSimulator.h"

#include <QGuiApplication>
#include <QScreen>
#include <cmath>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

ColorMatcher::ColorMatcher(QObject* parent)
    : QObject(parent)
{
    m_scanTimer = new QTimer(this);
    m_scanTimer->setTimerType(Qt::PreciseTimer);
    connect(m_scanTimer, &QTimer::timeout, this, &ColorMatcher::onScanTimer);
}

void ColorMatcher::setConfig(const ColorMatchConfig& config)
{
    m_config = config;
}

void ColorMatcher::start()
{
    if (m_running) return;

    m_running      = true;
    m_colorVisible = false;
    m_scanCount    = 0;

    m_scanTimer->setInterval(m_config.scanIntervalMs);
    m_scanTimer->start();

    LOG_INFO(tr("Поиск цвета запущен: RGB(%1,%2,%3), допуск=%4")
        .arg(m_config.targetColor.red())
        .arg(m_config.targetColor.green())
        .arg(m_config.targetColor.blue())
        .arg(m_config.tolerance));

    emit started();
}

void ColorMatcher::stop()
{
    if (!m_running) return;

    m_running = false;
    m_scanTimer->stop();

    LOG_INFO(tr("Поиск цвета остановлен (сканирований: %1)").arg(m_scanCount));
    emit stopped();
}

bool ColorMatcher::colorsMatch(const QColor& a, const QColor& b, int tolerance)
{
    // Сравнение в пространстве RGB с допуском
    int dr = std::abs(a.red()   - b.red());
    int dg = std::abs(a.green() - b.green());
    int db = std::abs(a.blue()  - b.blue());

    return (dr <= tolerance) && (dg <= tolerance) && (db <= tolerance);
}

bool ColorMatcher::findColor(QPoint& foundPos)
{
    QRect area;
    QImage image;

    // Определяем область поиска
    if (m_windowFinder && m_windowFinder->isTargetValid()) {
        // Захват окна
        QPixmap pm = m_windowFinder->captureWindow();
        if (pm.isNull()) return false;
        image = pm.toImage();
        area  = QRect(0, 0, image.width(), image.height());
    } else if (m_config.searchFullScreen) {
        // Весь экран
        QScreen* screen = QGuiApplication::primaryScreen();
        if (!screen) return false;
        QPixmap pm = screen->grabWindow(0);
        image = pm.toImage();
        area  = QRect(0, 0, image.width(), image.height());
    } else {
        // Заданная область
        area = m_config.searchArea;
        QScreen* screen = QGuiApplication::primaryScreen();
        if (!screen) return false;
        QPixmap pm = screen->grabWindow(0, area.x(), area.y(),
                                         area.width(), area.height());
        image = pm.toImage();
        area  = QRect(0, 0, image.width(), image.height());
    }

    return scanForColor(image, area, foundPos);
}

bool ColorMatcher::scanForColor(const QImage& image, const QRect& area, QPoint& foundPos)
{
    if (image.isNull()) return false;

    // Оптимизированный поиск: шаг сканирования зависит от размера области
    int step = 1;
    int totalPixels = area.width() * area.height();
    if (totalPixels > 1000000) step = 3;       // >1M пикселей — шаг 3
    else if (totalPixels > 500000) step = 2;   // >500K — шаг 2

    for (int y = area.top(); y < area.bottom(); y += step) {
        for (int x = area.left(); x < area.right(); x += step) {
            if (x < 0 || y < 0 || x >= image.width() || y >= image.height())
                continue;

            QColor pixel = image.pixelColor(x, y);
            if (colorsMatch(pixel, m_config.targetColor, m_config.tolerance)) {
                // Преобразуем координаты обратно в экранные
                if (m_windowFinder && m_windowFinder->isTargetValid()) {
                    foundPos = m_windowFinder->clientToScreen(QPoint(x, y));
                } else if (!m_config.searchFullScreen) {
                    foundPos = QPoint(m_config.searchArea.x() + x,
                                      m_config.searchArea.y() + y);
                } else {
                    foundPos = QPoint(x, y);
                }
                return true;
            }
        }
    }
    return false;
}

void ColorMatcher::onScanTimer()
{
    if (!m_running) return;

    m_scanCount++;
    QPoint foundPos;
    bool found = findColor(foundPos);

    if (found) {
        if (!m_colorVisible || !m_config.waitForDisappear) {
            // Цвет найден впервые (или не ждём исчезновения)
            m_colorVisible = true;
            emit colorFound(foundPos);

            // Выполняем действие
            InputSimulator sim;
            switch (m_config.action) {
                case ColorMatchConfig::Action::Click:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    sim.mouseClick(0); // ЛКМ
                    LOG_INFO(tr("Клик по цвету: (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;

                case ColorMatchConfig::Action::DoubleClick:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    sim.mouseDoubleClick(0);
                    LOG_INFO(tr("Двойной клик по цвету: (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;

                case ColorMatchConfig::Action::MoveTo:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    LOG_INFO(tr("Курсор перемещён к цвету: (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;

                case ColorMatchConfig::Action::Notify:
                    LOG_INFO(tr("Цвет обнаружен на (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;
            }
        }
    } else {
        if (m_colorVisible) {
            // Цвет исчез
            m_colorVisible = false;
            emit colorDisappeared();
            LOG_DEBUG(tr("Целевой цвет исчез с экрана"));
        }
    }

    emit scanCompleted(m_scanCount);
}

