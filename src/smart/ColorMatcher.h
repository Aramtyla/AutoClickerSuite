#pragma once
// ===========================================
// ColorMatcher.h — Поиск пикселя заданного цвета
// на экране с настраиваемым допуском
// ===========================================

#include <QObject>
#include <QColor>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QImage>

#include "SmartConfig.h"

class WindowFinder;

class ColorMatcher : public QObject
{
    Q_OBJECT

public:
    explicit ColorMatcher(QObject* parent = nullptr);

    // Установить конфигурацию
    void setConfig(const ColorMatchConfig& config);
    ColorMatchConfig config() const { return m_config; }

    // Установить WindowFinder для привязки к окну
    void setWindowFinder(WindowFinder* finder) { m_windowFinder = finder; }

    // Запуск/остановка сканирования
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // Разовый поиск (без запуска цикла)
    bool findColor(QPoint& foundPos);

    // Проверить, совпадает ли цвет пикселя (с допуском)
    static bool colorsMatch(const QColor& a, const QColor& b, int tolerance);

signals:
    // Цвет найден на позиции
    void colorFound(const QPoint& position);

    // Цвет исчез (если waitForDisappear)
    void colorDisappeared();

    // Статистика: количество сканирований
    void scanCompleted(int scanCount);

    void started();
    void stopped();

private slots:
    void onScanTimer();

private:
    // Захват области и поиск цвета
    bool scanForColor(const QImage& image, const QRect& area, QPoint& foundPos);

    ColorMatchConfig  m_config;
    WindowFinder*     m_windowFinder = nullptr;
    QTimer*           m_scanTimer    = nullptr;
    bool              m_running      = false;
    bool              m_colorVisible = false;  // Флаг: цвет видим в прошлом скане
    int               m_scanCount    = 0;
};

