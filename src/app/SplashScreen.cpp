// ===========================================
// SplashScreen.cpp — Анимированный экран загрузки
// ===========================================

#include "SplashScreen.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QEasingCurve>
#include <QFont>
#include <QtMath>

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    setFixedSize(540, 400);

    if (auto* screen = QApplication::primaryScreen()) {
        QRect screenRect = screen->availableGeometry();
        move((screenRect.width() - width()) / 2,
             (screenRect.height() - height()) / 2);
    }

    m_statusMessages = {
        QStringLiteral("Инициализация ядра..."),
        QStringLiteral("Загрузка модулей ввода..."),
        QStringLiteral("Настройка глобальных хоткеев..."),
        QStringLiteral("Загрузка тем оформления..."),
        QStringLiteral("Инициализация макро-движка..."),
        QStringLiteral("Подготовка интерфейса..."),
        QStringLiteral("Почти готово..."),
        QStringLiteral("Добро пожаловать!")
    };

    setupUI();
}

void SplashScreen::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 0, 40, 28);
    layout->setSpacing(0);

    // Пространство для логотипа (рисуется в paintEvent)
    layout->addSpacing(170);

    // === Название ===
    m_titleLabel = new QLabel("AutoClicker Suite", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "font-size: 26pt; font-weight: 700; color: #cdd6f4; "
        "background: transparent; letter-spacing: 1px;"
    );

    m_titleEffect = new QGraphicsOpacityEffect(m_titleLabel);
    m_titleEffect->setOpacity(0.0);
    m_titleLabel->setGraphicsEffect(m_titleEffect);

    layout->addWidget(m_titleLabel);
    layout->addSpacing(8);

    // === Команда ===
    m_teamLabel = new QLabel("Auto Clicker Suite Team", this);
    m_teamLabel->setAlignment(Qt::AlignCenter);
    m_teamLabel->setStyleSheet(
        "font-size: 10pt; color: #7f849c; background: transparent; "
        "letter-spacing: 3px; font-weight: 400;"
    );

    m_teamEffect = new QGraphicsOpacityEffect(m_teamLabel);
    m_teamEffect->setOpacity(0.0);
    m_teamLabel->setGraphicsEffect(m_teamEffect);

    layout->addWidget(m_teamLabel);
    layout->addSpacing(4);

    // === Версия ===
    m_versionLabel = new QLabel(QString("v%1").arg(APP_VERSION), this);
    m_versionLabel->setAlignment(Qt::AlignCenter);
    m_versionLabel->setStyleSheet(
        "font-size: 9pt; color: #585b70; background: transparent;"
    );

    m_versionEffect = new QGraphicsOpacityEffect(m_versionLabel);
    m_versionEffect->setOpacity(0.0);
    m_versionLabel->setGraphicsEffect(m_versionEffect);

    layout->addWidget(m_versionLabel);
    layout->addStretch();

    // === Статус ===
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(
        "font-size: 8pt; color: #585b70; background: transparent; "
        "padding-bottom: 8px;"
    );

    m_statusEffect = new QGraphicsOpacityEffect(m_statusLabel);
    m_statusEffect->setOpacity(0.0);
    m_statusLabel->setGraphicsEffect(m_statusEffect);

    layout->addWidget(m_statusLabel);

    // === Прогресс-бар ===
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(3);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(49, 50, 68, 120);"
        "  border: none;"
        "  border-radius: 1px;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #89b4fa, stop:0.5 #b4befe, stop:1 #cba6f7);"
        "  border-radius: 1px;"
        "}"
    );

    m_progressEffect = new QGraphicsOpacityEffect(m_progressBar);
    m_progressEffect->setOpacity(0.0);
    m_progressBar->setGraphicsEffect(m_progressEffect);

    layout->addWidget(m_progressBar);
}

// ===========================================
// Программная отрисовка векторного логотипа
// ===========================================

void SplashScreen::drawLogo(QPainter& p, const QRectF& rect)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF center = rect.center();
    qreal size = qMin(rect.width(), rect.height()) * 0.85;
    qreal half = size / 2.0;

    // === Фоновый круг с градиентом ===
    QRadialGradient circleBg(center, half);
    circleBg.setColorAt(0.0, QColor(137, 180, 250, 40));
    circleBg.setColorAt(0.6, QColor(137, 180, 250, 15));
    circleBg.setColorAt(1.0, QColor(137, 180, 250, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(circleBg);
    p.drawEllipse(center, half, half);

    // === Основное кольцо ===
    qreal ringRadius = half * 0.72;
    qreal ringWidth = size * 0.045;

    QPen ringPen(QColor(137, 180, 250, 200), ringWidth);
    ringPen.setCapStyle(Qt::RoundCap);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, ringRadius, ringRadius);

    // === Курсор-стрелка (стилизованная) ===
    qreal cursorSize = size * 0.32;
    QPointF cursorOrigin(center.x() - cursorSize * 0.1,
                         center.y() - cursorSize * 0.15);

    QPainterPath cursor;
    cursor.moveTo(cursorOrigin);
    cursor.lineTo(cursorOrigin.x(), cursorOrigin.y() + cursorSize);
    cursor.lineTo(cursorOrigin.x() + cursorSize * 0.30, cursorOrigin.y() + cursorSize * 0.72);
    cursor.lineTo(cursorOrigin.x() + cursorSize * 0.52, cursorOrigin.y() + cursorSize * 0.95);
    cursor.lineTo(cursorOrigin.x() + cursorSize * 0.62, cursorOrigin.y() + cursorSize * 0.82);
    cursor.lineTo(cursorOrigin.x() + cursorSize * 0.40, cursorOrigin.y() + cursorSize * 0.60);
    cursor.lineTo(cursorOrigin.x() + cursorSize * 0.72, cursorOrigin.y() + cursorSize * 0.60);
    cursor.closeSubpath();

    QLinearGradient cursorGrad(cursorOrigin,
                               QPointF(cursorOrigin.x() + cursorSize * 0.5,
                                       cursorOrigin.y() + cursorSize));
    cursorGrad.setColorAt(0.0, QColor(205, 214, 244));
    cursorGrad.setColorAt(1.0, QColor(180, 190, 254));
    p.setPen(Qt::NoPen);
    p.setBrush(cursorGrad);
    p.drawPath(cursor);

    p.setPen(QPen(QColor(137, 180, 250, 120), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(cursor);

    // === Декоративные точки клика ===
    auto drawClickDot = [&](qreal angle, qreal dist, qreal dotSize, int alpha) {
        qreal rad = qDegreesToRadians(angle);
        QPointF dotPos(center.x() + dist * qCos(rad),
                       center.y() + dist * qSin(rad));
        QRadialGradient dotGrad(dotPos, dotSize);
        dotGrad.setColorAt(0.0, QColor(166, 227, 161, alpha));
        dotGrad.setColorAt(1.0, QColor(166, 227, 161, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(dotGrad);
        p.drawEllipse(dotPos, dotSize, dotSize);

        p.setBrush(QColor(166, 227, 161, alpha));
        p.drawEllipse(dotPos, dotSize * 0.3, dotSize * 0.3);
    };

    drawClickDot(-45, ringRadius * 0.95, size * 0.06, 180);
    drawClickDot(200, ringRadius * 0.85, size * 0.04, 130);
    drawClickDot(90, ringRadius * 1.05, size * 0.035, 100);

    p.restore();
}

void SplashScreen::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r(0, 0, width(), height());

    // === Glow effect за окном ===
    if (m_glowRadius > 0) {
        QRadialGradient glow(r.center().x(), r.height() * 0.35, m_glowRadius);
        glow.setColorAt(0.0, QColor(137, 180, 250, 30));
        glow.setColorAt(0.4, QColor(180, 190, 254, 10));
        glow.setColorAt(1.0, QColor(137, 180, 250, 0));
        p.fillRect(r, glow);
    }

    // === Основной фон — закруглённый прямоугольник ===
    QRectF cardRect = r.adjusted(12, 12, -12, -12);
    QPainterPath path;
    path.addRoundedRect(cardRect, 20, 20);

    // Фон
    QColor bgColor(17, 17, 27);  // #11111b
    bgColor.setAlphaF(m_bgOpacity * 0.97);
    p.fillPath(path, bgColor);

    // Рамка с градиентом
    if (m_bgOpacity > 0.1) {
        QLinearGradient borderGrad(cardRect.topLeft(), cardRect.bottomRight());
        QColor bc1(137, 180, 250, static_cast<int>(60 * m_bgOpacity));
        QColor bc2(49, 50, 68, static_cast<int>(100 * m_bgOpacity));
        borderGrad.setColorAt(0.0, bc1);
        borderGrad.setColorAt(0.5, bc2);
        borderGrad.setColorAt(1.0, bc1);
        p.setPen(QPen(QBrush(borderGrad), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    // Верхний акцентный градиент
    if (m_bgOpacity > 0.3) {
        QLinearGradient topGlow(cardRect.left(), cardRect.top(),
                                cardRect.left(), cardRect.top() + 120);
        topGlow.setColorAt(0.0, QColor(137, 180, 250, static_cast<int>(18 * m_bgOpacity)));
        topGlow.setColorAt(0.5, QColor(180, 190, 254, static_cast<int>(5 * m_bgOpacity)));
        topGlow.setColorAt(1.0, QColor(0, 0, 0, 0));

        QPainterPath topPath;
        topPath.addRoundedRect(cardRect.adjusted(1, 1, -1, -(cardRect.height() - 120)), 19, 19);
        p.fillPath(topPath, topGlow);
    }

    // === Логотип (векторная отрисовка — всегда чёткий) ===
    if (m_iconOpacity > 0.01) {
        p.setOpacity(m_iconOpacity);

        QRectF logoRect(r.center().x() - 50, 45, 100, 100);

        if (m_iconScale < 1.0 || m_iconScale > 1.0) {
            QPointF lc = logoRect.center();
            p.translate(lc);
            p.scale(m_iconScale, m_iconScale);
            p.translate(-lc);
        }

        drawLogo(p, logoRect);

        p.resetTransform();
        p.setOpacity(1.0);
    }
}

void SplashScreen::start()
{
    show();
    startAnimations();
}

void SplashScreen::startAnimations()
{
    m_mainSequence = new QSequentialAnimationGroup(this);

    // === Фаза 1: фон появляется (450 мс) ===
    auto* bgAnim = new QPropertyAnimation(this, "backgroundOpacity", this);
    bgAnim->setDuration(450);
    bgAnim->setStartValue(0.0);
    bgAnim->setEndValue(1.0);
    bgAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_mainSequence->addAnimation(bgAnim);

    // === Фаза 2: glow + логотип (550 мс, параллельно) ===
    auto* phase2 = new QParallelAnimationGroup(this);

    auto* glowAnim = new QPropertyAnimation(this, "glowRadius", this);
    glowAnim->setDuration(650);
    glowAnim->setStartValue(0.0);
    glowAnim->setEndValue(static_cast<qreal>(width()));
    glowAnim->setEasingCurve(QEasingCurve::OutQuad);
    phase2->addAnimation(glowAnim);

    auto* iconOpacAnim = new QPropertyAnimation(this, "iconOpacity", this);
    iconOpacAnim->setDuration(550);
    iconOpacAnim->setStartValue(0.0);
    iconOpacAnim->setEndValue(1.0);
    iconOpacAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase2->addAnimation(iconOpacAnim);

    auto* iconScaleAnim = new QPropertyAnimation(this, "iconScale", this);
    iconScaleAnim->setDuration(600);
    iconScaleAnim->setStartValue(0.5);
    iconScaleAnim->setEndValue(1.0);
    iconScaleAnim->setEasingCurve(QEasingCurve::OutBack);
    phase2->addAnimation(iconScaleAnim);

    m_mainSequence->addAnimation(phase2);

    // === Фаза 3: название (400 мс) ===
    auto* titleAnim = new QPropertyAnimation(m_titleEffect, "opacity", this);
    titleAnim->setDuration(400);
    titleAnim->setStartValue(0.0);
    titleAnim->setEndValue(1.0);
    titleAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_mainSequence->addAnimation(titleAnim);

    // === Фаза 4: команда + версия + прогресс (350 мс) ===
    auto* phase4 = new QParallelAnimationGroup(this);

    auto* teamAnim = new QPropertyAnimation(m_teamEffect, "opacity", this);
    teamAnim->setDuration(350);
    teamAnim->setStartValue(0.0);
    teamAnim->setEndValue(1.0);
    teamAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase4->addAnimation(teamAnim);

    auto* verAnim = new QPropertyAnimation(m_versionEffect, "opacity", this);
    verAnim->setDuration(350);
    verAnim->setStartValue(0.0);
    verAnim->setEndValue(1.0);
    verAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase4->addAnimation(verAnim);

    auto* progAnim = new QPropertyAnimation(m_progressEffect, "opacity", this);
    progAnim->setDuration(350);
    progAnim->setStartValue(0.0);
    progAnim->setEndValue(1.0);
    progAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase4->addAnimation(progAnim);

    auto* statusAnim = new QPropertyAnimation(m_statusEffect, "opacity", this);
    statusAnim->setDuration(350);
    statusAnim->setStartValue(0.0);
    statusAnim->setEndValue(1.0);
    statusAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase4->addAnimation(statusAnim);

    m_mainSequence->addAnimation(phase4);

    m_mainSequence->start();

    // === Прогресс-бар ===
    m_progressTimer = new QTimer(this);
    m_progressValue = 0;
    m_statusIndex = 0;
    m_statusLabel->setText(m_statusMessages.first());

    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        m_progressValue += 2;
        m_progressBar->setValue(m_progressValue);

        int newIndex = (m_progressValue * m_statusMessages.size()) / 105;
        if (newIndex != m_statusIndex && newIndex < m_statusMessages.size()) {
            m_statusIndex = newIndex;
            m_statusLabel->setText(m_statusMessages[m_statusIndex]);
        }

        if (m_progressValue >= 100) {
            m_progressTimer->stop();
            m_statusLabel->setText(m_statusMessages.last());
            QTimer::singleShot(400, this, &SplashScreen::fadeOut);
        }
    });

    QTimer::singleShot(1300, this, [this]() {
        m_progressTimer->start(30);
    });
}

void SplashScreen::fadeOut()
{
    auto* fadeGroup = new QParallelAnimationGroup(this);

    auto* fadeAnim = new QPropertyAnimation(this, "backgroundOpacity", this);
    fadeAnim->setDuration(500);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::InCubic);
    fadeGroup->addAnimation(fadeAnim);

    auto* iconFade = new QPropertyAnimation(this, "iconOpacity", this);
    iconFade->setDuration(400);
    iconFade->setStartValue(m_iconOpacity);
    iconFade->setEndValue(0.0);
    iconFade->setEasingCurve(QEasingCurve::InCubic);
    fadeGroup->addAnimation(iconFade);

    auto* iconScaleOut = new QPropertyAnimation(this, "iconScale", this);
    iconScaleOut->setDuration(500);
    iconScaleOut->setStartValue(m_iconScale);
    iconScaleOut->setEndValue(1.15);
    iconScaleOut->setEasingCurve(QEasingCurve::InCubic);
    fadeGroup->addAnimation(iconScaleOut);

    auto addFade = [&](QGraphicsOpacityEffect* effect) {
        auto* a = new QPropertyAnimation(effect, "opacity", this);
        a->setDuration(400);
        a->setStartValue(effect->opacity());
        a->setEndValue(0.0);
        a->setEasingCurve(QEasingCurve::InCubic);
        fadeGroup->addAnimation(a);
    };

    addFade(m_titleEffect);
    addFade(m_teamEffect);
    addFade(m_versionEffect);
    addFade(m_progressEffect);
    addFade(m_statusEffect);

    auto* glowFade = new QPropertyAnimation(this, "glowRadius", this);
    glowFade->setDuration(500);
    glowFade->setStartValue(m_glowRadius);
    glowFade->setEndValue(0.0);
    glowFade->setEasingCurve(QEasingCurve::InCubic);
    fadeGroup->addAnimation(glowFade);

    connect(fadeGroup, &QAbstractAnimation::finished, this, [this]() {
        emit finished();
        close();
    });

    fadeGroup->start(QAbstractAnimation::DeleteWhenStopped);
}
