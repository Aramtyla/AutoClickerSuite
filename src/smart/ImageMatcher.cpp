// ===========================================
// ImageMatcher.cpp — Template Matching без OpenCV
// Нормализованная кросс-корреляция (NCC) с
// пирамидальной оптимизацией для ускорения поиска.
// ===========================================

// Отключаем макросы min/max из Windows.h
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ImageMatcher.h"
#include "WindowFinder.h"
#include "utils/Logger.h"
#include "core/InputSimulator.h"

#include <QGuiApplication>
#include <QScreen>
#include <cmath>
#include <algorithm>

ImageMatcher::ImageMatcher(QObject* parent)
    : QObject(parent)
{
    m_scanTimer = new QTimer(this);
    m_scanTimer->setTimerType(Qt::PreciseTimer);
    connect(m_scanTimer, &QTimer::timeout, this, &ImageMatcher::onScanTimer);
}

void ImageMatcher::setConfig(const ImageMatchConfig& config)
{
    m_config = config;
}

bool ImageMatcher::loadTemplate(const QString& filePath)
{
    QImage img(filePath);
    if (img.isNull()) {
        LOG_ERROR(tr("Не удалось загрузить шаблон: %1").arg(filePath));
        return false;
    }

    setTemplate(img);
    m_config.templatePath = filePath;
    LOG_INFO(tr("Шаблон загружен: %1 (%2x%3)")
        .arg(filePath).arg(img.width()).arg(img.height()));
    return true;
}

void ImageMatcher::setTemplate(const QImage& templateImage)
{
    m_templateImage = templateImage.convertToFormat(QImage::Format_ARGB32);
    if (m_config.useGrayscale) {
        m_templateGray = toGrayscale(m_templateImage);
    }
}

void ImageMatcher::start()
{
    if (m_running) return;
    if (m_templateImage.isNull()) {
        LOG_ERROR(tr("Шаблон не загружен — невозможно начать поиск"));
        return;
    }

    m_running      = true;
    m_imageVisible = false;
    m_scanCount    = 0;

    m_scanTimer->setInterval(m_config.scanIntervalMs);
    m_scanTimer->start();

    LOG_INFO(tr("Поиск изображения запущен (шаблон %1x%2, порог %.2f)")
        .arg(m_templateImage.width())
        .arg(m_templateImage.height())
        .arg(m_config.threshold));

    emit started();
}

void ImageMatcher::stop()
{
    if (!m_running) return;

    m_running = false;
    m_scanTimer->stop();

    LOG_INFO(tr("Поиск изображения остановлен (сканирований: %1)").arg(m_scanCount));
    emit stopped();
}

bool ImageMatcher::findImage(QPoint& foundPos, double& matchScore)
{
    // Захват экрана/окна
    QImage screenImage;

    if (m_windowFinder && m_windowFinder->isTargetValid()) {
        QPixmap pm = m_windowFinder->captureWindow();
        if (pm.isNull()) return false;
        screenImage = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    } else if (m_config.searchFullScreen) {
        QScreen* screen = QGuiApplication::primaryScreen();
        if (!screen) return false;
        screenImage = screen->grabWindow(0).toImage().convertToFormat(QImage::Format_ARGB32);
    } else {
        QScreen* screen = QGuiApplication::primaryScreen();
        if (!screen) return false;
        QRect a = m_config.searchArea;
        screenImage = screen->grabWindow(0, a.x(), a.y(), a.width(), a.height())
                              .toImage().convertToFormat(QImage::Format_ARGB32);
    }

    if (screenImage.isNull()) return false;

    // Используем grayscale для ускорения, если включено
    QImage searchImg = m_config.useGrayscale ? toGrayscale(screenImage) : screenImage;
    QImage templImg  = m_config.useGrayscale ? m_templateGray : m_templateImage;

    QPoint bestPos;
    double bestScore;

    if (!templateMatch(searchImg, templImg, bestPos, bestScore)) {
        return false;
    }

    if (bestScore >= m_config.threshold) {
        // Преобразуем координаты в экранные
        int cx = bestPos.x() + templImg.width()  / 2;
        int cy = bestPos.y() + templImg.height() / 2;

        if (m_windowFinder && m_windowFinder->isTargetValid()) {
            foundPos = m_windowFinder->clientToScreen(QPoint(cx, cy));
        } else if (!m_config.searchFullScreen) {
            foundPos = QPoint(m_config.searchArea.x() + cx,
                               m_config.searchArea.y() + cy);
        } else {
            foundPos = QPoint(cx, cy);
        }

        matchScore = bestScore;
        return true;
    }

    return false;
}

// ==========================================
// Template Matching — Нормализованная кросс-корреляция
// ==========================================
bool ImageMatcher::templateMatch(const QImage& screen, const QImage& templ,
                                  QPoint& bestPos, double& bestScore)
{
    int sw = screen.width();
    int sh = screen.height();
    int tw = templ.width();
    int th = templ.height();

    if (tw > sw || th > sh) return false;
    if (tw == 0 || th == 0) return false;

    // Пирамидальная оптимизация: сначала ищем на уменьшенном изображении
    QPoint candidatePos(-1, -1);
    bool hasCandidateFromPyramid = false;

    if (sw > 400 && sh > 400 && tw > 20 && th > 20) {
        hasCandidateFromPyramid = quickCheck(screen, templ, candidatePos);
    }

    bestScore = -1.0;
    bestPos   = QPoint(0, 0);

    // Определяем область поиска
    int startX = 0, endX = sw - tw;
    int startY = 0, endY = sh - th;
    int step   = 1;

    if (hasCandidateFromPyramid) {
        // Уточняем в окрестности кандидата
        int margin = std::max(tw, th);
        startX = std::max(0, candidatePos.x() - margin);
        startY = std::max(0, candidatePos.y() - margin);
        endX   = std::min(sw - tw, candidatePos.x() + margin);
        endY   = std::min(sh - th, candidatePos.y() + margin);
    } else if ((sw - tw) * (sh - th) > 500000) {
        // Большая область — ищем с шагом 2
        step = 2;
    }

    // Предвычисляем среднее шаблона
    double templMean = 0.0;
    int totalPixels = tw * th;

    for (int y = 0; y < th; ++y) {
        const uchar* line = templ.constScanLine(y);
        for (int x = 0; x < tw; ++x) {
            // Для grayscale: R == G == B, берём R
            templMean += line[x * 4 + 2]; // R channel в ARGB32
        }
    }
    templMean /= totalPixels;

    // Дисперсия шаблона
    double templVar = 0.0;
    for (int y = 0; y < th; ++y) {
        const uchar* line = templ.constScanLine(y);
        for (int x = 0; x < tw; ++x) {
            double diff = line[x * 4 + 2] - templMean;
            templVar += diff * diff;
        }
    }
    if (templVar < 1.0) return false; // Шаблон однотонный

    double templStd = std::sqrt(templVar);

    // Основной цикл сканирования
    for (int sy = startY; sy <= endY; sy += step) {
        for (int sx = startX; sx <= endX; sx += step) {
            // Вычисляем NCC для позиции (sx, sy)
            double screenMean = 0.0;

            for (int ty = 0; ty < th; ++ty) {
                const uchar* sLine = screen.constScanLine(sy + ty);
                for (int tx = 0; tx < tw; ++tx) {
                    screenMean += sLine[(sx + tx) * 4 + 2];
                }
            }
            screenMean /= totalPixels;

            // Кросс-корреляция и дисперсия участка экрана
            double crossCorr = 0.0;
            double screenVar = 0.0;

            for (int ty = 0; ty < th; ++ty) {
                const uchar* sLine = screen.constScanLine(sy + ty);
                const uchar* tLine = templ.constScanLine(ty);
                for (int tx = 0; tx < tw; ++tx) {
                    double sVal = sLine[(sx + tx) * 4 + 2] - screenMean;
                    double tVal = tLine[tx * 4 + 2]        - templMean;
                    crossCorr += sVal * tVal;
                    screenVar += sVal * sVal;
                }
            }

            if (screenVar < 1.0) continue; // Однотонная область

            double ncc = crossCorr / (std::sqrt(screenVar) * templStd);

            if (ncc > bestScore) {
                bestScore = ncc;
                bestPos   = QPoint(sx, sy);

                // Ранний выход — уже нашли очень хорошее совпадение
                if (bestScore > 0.98) return true;
            }
        }
    }

    return bestScore > 0.0;
}

bool ImageMatcher::quickCheck(const QImage& screen, const QImage& templ,
                               QPoint& candidatePos)
{
    // Уменьшаем изображения в 4 раза для предварительного поиска
    int factor = 4;
    QImage smallScreen = downsample(screen, factor);
    QImage smallTempl  = downsample(templ, factor);

    if (smallTempl.width() < 4 || smallTempl.height() < 4)
        return false;

    QPoint bestPos;
    double bestScore;
    if (templateMatch(smallScreen, smallTempl, bestPos, bestScore)) {
        if (bestScore > m_config.threshold * 0.7) {
            // Масштабируем координаты обратно
            candidatePos = QPoint(bestPos.x() * factor, bestPos.y() * factor);
            return true;
        }
    }
    return false;
}

QImage ImageMatcher::toGrayscale(const QImage& image)
{
    QImage gray(image.width(), image.height(), QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y) {
        const uchar* srcLine = image.constScanLine(y);
        uchar*       dstLine = gray.scanLine(y);

        for (int x = 0; x < image.width(); ++x) {
            int idx = x * 4;
            int b = srcLine[idx + 0];
            int g = srcLine[idx + 1];
            int r = srcLine[idx + 2];
            int a = srcLine[idx + 3];

            // ITU-R BT.601 коэффициенты
            int luma = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);

            dstLine[idx + 0] = luma; // B
            dstLine[idx + 1] = luma; // G
            dstLine[idx + 2] = luma; // R
            dstLine[idx + 3] = a;    // A
        }
    }
    return gray;
}

QImage ImageMatcher::downsample(const QImage& image, int factor)
{
    int nw = image.width()  / factor;
    int nh = image.height() / factor;
    if (nw < 1 || nh < 1) return image;

    return image.scaled(nw, nh, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void ImageMatcher::onScanTimer()
{
    if (!m_running) return;

    m_scanCount++;
    QPoint foundPos;
    double score = 0.0;
    bool found = findImage(foundPos, score);

    if (found) {
        if (!m_imageVisible || !m_config.waitForDisappear) {
            m_imageVisible = true;
            emit imageFound(foundPos, score);

            // Выполняем действие
            InputSimulator sim;
            switch (m_config.action) {
                case ImageMatchConfig::Action::ClickCenter:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    sim.mouseClick(0);
                    LOG_INFO(tr("Клик по изображению: (%1, %2), совпадение: %3%")
                        .arg(foundPos.x()).arg(foundPos.y())
                        .arg(static_cast<int>(score * 100)));
                    break;

                case ImageMatchConfig::Action::DoubleClick:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    sim.mouseDoubleClick(0);
                    LOG_INFO(tr("Двойной клик по изображению: (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;

                case ImageMatchConfig::Action::MoveTo:
                    sim.mouseMoveTo(foundPos.x(), foundPos.y());
                    LOG_INFO(tr("Курсор перемещён к изображению: (%1, %2)")
                        .arg(foundPos.x()).arg(foundPos.y()));
                    break;

                case ImageMatchConfig::Action::Notify:
                    LOG_INFO(tr("Изображение найдено на (%1, %2), совпадение: %3%")
                        .arg(foundPos.x()).arg(foundPos.y())
                        .arg(static_cast<int>(score * 100)));
                    break;
            }
        }
    } else {
        if (m_imageVisible) {
            m_imageVisible = false;
            emit imageDisappeared();
            LOG_DEBUG(tr("Целевое изображение исчезло с экрана"));
        }
    }

    emit scanCompleted(m_scanCount);
}

