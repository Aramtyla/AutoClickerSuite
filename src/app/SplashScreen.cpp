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

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent)
{
    // Полноценное окно без рамки, поверх всех
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // Размер splash — фиксированный
    setFixedSize(520, 380);

    // Центрируем на экране
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
    layout->setContentsMargins(40, 45, 40, 30);
    layout->setSpacing(0);

    // === Иконка ===
    m_iconLabel = new QLabel(this);
    QPixmap icon(":/icons/app.png");
    if (!icon.isNull()) {
        m_iconLabel->setPixmap(icon.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedHeight(110);

    m_iconEffect = new QGraphicsOpacityEffect(m_iconLabel);
    m_iconEffect->setOpacity(0.0);
    m_iconLabel->setGraphicsEffect(m_iconEffect);

    layout->addWidget(m_iconLabel);

    // === Название ===
    m_titleLabel = new QLabel("AutoClicker Suite", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "font-size: 28pt; font-weight: bold; color: #89b4fa; "
        "background: transparent; letter-spacing: 1px;"
    );

    m_titleEffect = new QGraphicsOpacityEffect(m_titleLabel);
    m_titleEffect->setOpacity(0.0);
    m_titleLabel->setGraphicsEffect(m_titleEffect);

    layout->addWidget(m_titleLabel);
    layout->addSpacing(6);

    // === Команда ===
    m_teamLabel = new QLabel("Auto Clicker Suite Team", this);
    m_teamLabel->setAlignment(Qt::AlignCenter);
    m_teamLabel->setStyleSheet(
        "font-size: 11pt; color: #a6adc8; background: transparent; "
        "letter-spacing: 2px; font-weight: 300;"
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
        "font-size: 9pt; color: #6c7086; background: transparent;"
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
        "font-size: 9pt; color: #6c7086; background: transparent; "
        "padding-bottom: 6px;"
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
    m_progressBar->setFixedHeight(4);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(49, 50, 68, 180);"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #89b4fa, stop:0.5 #b4befe, stop:1 #89b4fa);"
        "  border-radius: 2px;"
        "}"
    );

    m_progressEffect = new QGraphicsOpacityEffect(m_progressBar);
    m_progressEffect->setOpacity(0.0);
    m_progressBar->setGraphicsEffect(m_progressEffect);

    layout->addWidget(m_progressBar);
}

void SplashScreen::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r(0, 0, width(), height());

    // === Glow effect (подсветка за окном) ===
    if (m_glowRadius > 0) {
        QRadialGradient glow(r.center(), m_glowRadius);
        glow.setColorAt(0.0, QColor(137, 180, 250, 35));  // #89b4fa
        glow.setColorAt(0.5, QColor(137, 180, 250, 12));
        glow.setColorAt(1.0, QColor(137, 180, 250, 0));
        p.fillRect(r, glow);
    }

    // === Основной фон — закруглённый прямоугольник ===
    QRectF cardRect = r.adjusted(10, 10, -10, -10);
    QPainterPath path;
    path.addRoundedRect(cardRect, 18, 18);

    // Фон с прозрачностью
    QColor bgColor(24, 24, 37);  // #181825
    bgColor.setAlphaF(m_bgOpacity * 0.95);
    p.fillPath(path, bgColor);

    // Тонкая рамка
    QColor borderColor(49, 50, 68);  // #313244
    borderColor.setAlphaF(m_bgOpacity * 0.7);
    p.setPen(QPen(borderColor, 1.0));
    p.drawPath(path);

    // Верхний акцентный градиент
    if (m_bgOpacity > 0.3) {
        QLinearGradient topGlow(cardRect.left(), cardRect.top(),
                                cardRect.left(), cardRect.top() + 80);
        topGlow.setColorAt(0.0, QColor(137, 180, 250, static_cast<int>(20 * m_bgOpacity)));
        topGlow.setColorAt(1.0, QColor(137, 180, 250, 0));

        QPainterPath topPath;
        topPath.addRoundedRect(cardRect.adjusted(0, 0, 0, -(cardRect.height() - 80)), 18, 18);
        p.fillPath(topPath, topGlow);
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

    // === Фаза 1: фон появляется (400 мс) ===
    auto* bgAnim = new QPropertyAnimation(this, "backgroundOpacity", this);
    bgAnim->setDuration(400);
    bgAnim->setStartValue(0.0);
    bgAnim->setEndValue(1.0);
    bgAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_mainSequence->addAnimation(bgAnim);

    // === Фаза 2: glow + иконка (500 мс, параллельно) ===
    auto* phase2 = new QParallelAnimationGroup(this);

    auto* glowAnim = new QPropertyAnimation(this, "glowRadius", this);
    glowAnim->setDuration(600);
    glowAnim->setStartValue(0.0);
    glowAnim->setEndValue(static_cast<qreal>(width()));
    glowAnim->setEasingCurve(QEasingCurve::OutQuad);
    phase2->addAnimation(glowAnim);

    auto* iconAnim = new QPropertyAnimation(m_iconEffect, "opacity", this);
    iconAnim->setDuration(500);
    iconAnim->setStartValue(0.0);
    iconAnim->setEndValue(1.0);
    iconAnim->setEasingCurve(QEasingCurve::OutCubic);
    phase2->addAnimation(iconAnim);

    m_mainSequence->addAnimation(phase2);

    // === Фаза 3: название (400 мс) ===
    auto* titleAnim = new QPropertyAnimation(m_titleEffect, "opacity", this);
    titleAnim->setDuration(400);
    titleAnim->setStartValue(0.0);
    titleAnim->setEndValue(1.0);
    titleAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_mainSequence->addAnimation(titleAnim);

    // === Фаза 4: команда + версия + прогресс (300 мс, параллельно) ===
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

    // Запуск последовательности
    m_mainSequence->start();

    // === Прогресс-бар ===
    m_progressTimer = new QTimer(this);
    m_progressValue = 0;
    m_statusIndex = 0;
    m_statusLabel->setText(m_statusMessages.first());

    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        m_progressValue += 2;
        m_progressBar->setValue(m_progressValue);

        // Обновляем статусное сообщение
        int newIndex = (m_progressValue * m_statusMessages.size()) / 105;
        if (newIndex != m_statusIndex && newIndex < m_statusMessages.size()) {
            m_statusIndex = newIndex;
            m_statusLabel->setText(m_statusMessages[m_statusIndex]);
        }

        if (m_progressValue >= 100) {
            m_progressTimer->stop();
            m_statusLabel->setText(m_statusMessages.last());

            // Задержка перед fadeOut
            QTimer::singleShot(400, this, &SplashScreen::fadeOut);
        }
    });

    // Запускаем прогресс после начальных анимаций (~1600 мс)
    QTimer::singleShot(1200, this, [this]() {
        m_progressTimer->start(30); // ~1.5 сек на весь прогресс
    });
}

void SplashScreen::fadeOut()
{
    // Плавное исчезновение всего окна
    auto* fadeAnim = new QPropertyAnimation(this, "backgroundOpacity", this);
    fadeAnim->setDuration(500);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::InCubic);

    // Делаем все элементы тоже постепенно прозрачными
    auto* fadeGroup = new QParallelAnimationGroup(this);
    fadeGroup->addAnimation(fadeAnim);

    auto addFade = [&](QGraphicsOpacityEffect* effect) {
        auto* a = new QPropertyAnimation(effect, "opacity", this);
        a->setDuration(400);
        a->setStartValue(effect->opacity());
        a->setEndValue(0.0);
        a->setEasingCurve(QEasingCurve::InCubic);
        fadeGroup->addAnimation(a);
    };

    addFade(m_iconEffect);
    addFade(m_titleEffect);
    addFade(m_teamEffect);
    addFade(m_versionEffect);
    addFade(m_progressEffect);
    addFade(m_statusEffect);

    // Glow shrink
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
