#pragma once
// ===========================================
// MouseConfig.h — Конфигурация модуля мыши
// ===========================================

#include <QPoint>

// Кнопка мыши
enum class MouseButton {
    Left,
    Right,
    Middle
};

// Тип клика
enum class ClickType {
    Single,
    Double,
    Hold  // Удержание кнопки
};

// Режим координат
enum class ClickPosition {
    FollowCursor,      // Клик в текущей позиции курсора
    FixedPosition,     // Фиксированные координаты
    MultiPoint         // Последовательность координат
};

// Структура конфигурации автоклика мыши
struct MouseClickerConfig
{
    MouseButton    button       = MouseButton::Left;
    ClickType      clickType    = ClickType::Single;
    ClickPosition  positionMode = ClickPosition::FollowCursor;

    int  intervalMs        = 100;    // Интервал между кликами (мс)
    int  clickCount        = 0;      // 0 = бесконечно
    int  holdDurationMs    = 100;    // Длительность удержания (для Hold)

    // Фиксированная позиция
    QPoint fixedPos        = {0, 0};

    // Рандомизация
    bool randomizeInterval = false;   // Случайный интервал
    int  randomIntervalMin = 50;
    int  randomIntervalMax = 200;

    bool randomizePosition = false;   // Случайное смещение координат
    int  randomOffsetPixels = 5;      // Радиус смещения

    // Ограничение по времени (мс), 0 = без ограничения
    int  timeLimitMs       = 0;
};
