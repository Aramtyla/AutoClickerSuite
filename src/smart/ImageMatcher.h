#pragma once
// ===========================================
// ImageMatcher.h — Template Matching без OpenCV
// Поиск фрагмента изображения на экране
// Реализация через попиксельное сравнение
// с оптимизацией (пирамида + ранний выход)
// ===========================================

#include <QObject>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QPixmap>

#include "SmartConfig.h"

class WindowFinder;

class ImageMatcher : public QObject
{
    Q_OBJECT

public:
    explicit ImageMatcher(QObject* parent = nullptr);

    // Конфигурация
    void setConfig(const ImageMatchConfig& config);
    ImageMatchConfig config() const { return m_config; }

    // Загрузить шаблон из файла
    bool loadTemplate(const QString& filePath);

    // Установить шаблон из QImage
    void setTemplate(const QImage& templateImage);

    // Привязка к окну
    void setWindowFinder(WindowFinder* finder) { m_windowFinder = finder; }

    // Запуск/остановка
    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // Разовый поиск
    bool findImage(QPoint& foundPos, double& matchScore);

signals:
    // Изображение найдено
    void imageFound(const QPoint& position, double score);

    // Изображение исчезло
    void imageDisappeared();

    // Сканирование завершено
    void scanCompleted(int scanCount);

    void started();
    void stopped();

private slots:
    void onScanTimer();

private:
    // Template matching — нормализованная кросс-корреляция (NCC)
    // Возвращает позицию лучшего совпадения и его score
    bool templateMatch(const QImage& screen, const QImage& templ,
                       QPoint& bestPos, double& bestScore);

    // Быстрая предварительная проверка (субдискретизация)
    bool quickCheck(const QImage& screen, const QImage& templ,
                    QPoint& candidatePos);

    // Преобразование в оттенки серого
    static QImage toGrayscale(const QImage& image);

    // Субдискретизация (уменьшение масштаба)
    static QImage downsample(const QImage& image, int factor);

    ImageMatchConfig  m_config;
    WindowFinder*     m_windowFinder = nullptr;
    QTimer*           m_scanTimer    = nullptr;
    QImage            m_templateImage;         // Шаблон
    QImage            m_templateGray;          // Шаблон в оттенках серого
    bool              m_running      = false;
    bool              m_imageVisible = false;
    int               m_scanCount    = 0;
};

