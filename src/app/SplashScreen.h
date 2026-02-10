#pragma once
// ===========================================
// SplashScreen.h — Анимированный экран загрузки
// ===========================================

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QLabel>
#include <QProgressBar>
#include <QGraphicsOpacityEffect>

class SplashScreen : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal backgroundOpacity READ backgroundOpacity WRITE setBackgroundOpacity)
    Q_PROPERTY(qreal glowRadius READ glowRadius WRITE setGlowRadius)
    Q_PROPERTY(qreal iconScale READ iconScale WRITE setIconScale)
    Q_PROPERTY(qreal iconOpacity READ iconOpacity WRITE setIconOpacity)

public:
    explicit SplashScreen(QWidget* parent = nullptr);
    ~SplashScreen() override = default;

    void start();

    qreal backgroundOpacity() const { return m_bgOpacity; }
    void setBackgroundOpacity(qreal v) { m_bgOpacity = v; update(); }

    qreal glowRadius() const { return m_glowRadius; }
    void setGlowRadius(qreal v) { m_glowRadius = v; update(); }

    qreal iconScale() const { return m_iconScale; }
    void setIconScale(qreal v) { m_iconScale = v; update(); }

    qreal iconOpacity() const { return m_iconOpacity; }
    void setIconOpacity(qreal v) { m_iconOpacity = v; update(); }

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void startAnimations();
    void fadeOut();
    void drawLogo(QPainter& p, const QRectF& rect);

    // UI — только текстовые элементы (логотип рисуется через QPainter)
    QLabel*       m_titleLabel   = nullptr;
    QLabel*       m_teamLabel    = nullptr;
    QLabel*       m_versionLabel = nullptr;
    QProgressBar* m_progressBar  = nullptr;
    QLabel*       m_statusLabel  = nullptr;

    // Effects
    QGraphicsOpacityEffect* m_titleEffect   = nullptr;
    QGraphicsOpacityEffect* m_teamEffect    = nullptr;
    QGraphicsOpacityEffect* m_versionEffect = nullptr;
    QGraphicsOpacityEffect* m_progressEffect = nullptr;
    QGraphicsOpacityEffect* m_statusEffect  = nullptr;

    // Animation
    QSequentialAnimationGroup* m_mainSequence = nullptr;
    QTimer* m_progressTimer = nullptr;
    int     m_progressValue = 0;

    // Custom paint properties
    qreal m_bgOpacity   = 0.0;
    qreal m_glowRadius  = 0.0;
    qreal m_iconScale   = 0.0;
    qreal m_iconOpacity = 0.0;

    // Status messages
    QStringList m_statusMessages;
    int m_statusIndex = 0;
};
