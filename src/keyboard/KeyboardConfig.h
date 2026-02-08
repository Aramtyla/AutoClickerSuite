#pragma once
// ===========================================
// KeyboardConfig.h — Конфигурация модуля клавиатуры
// ===========================================

#include <QString>
#include <QList>
#include <QPair>
#include <QJsonObject>
#include <QJsonArray>

// Режим работы клавиатурного модуля
enum class KeyboardMode {
    SingleKey,       // Повторение одной клавиши
    KeyCombination,  // Повторение комбинации (Ctrl+V, и т.д.)
    TypeText,        // Посимвольный ввод текста
    MacroPlayback    // Воспроизведение записанной последовательности
};

// Запись одного действия макроса клавиатуры
struct KeyboardMacroAction
{
    enum class Type {
        KeyDown,     // Нажатие клавиши
        KeyUp,       // Отпускание клавиши
        Delay        // Задержка
    };

    Type type          = Type::KeyDown;
    int  vkCode        = 0;          // VK-код (для KeyDown/KeyUp)
    int  delayMs       = 0;          // Задержка (для Delay)
    qint64 timestamp   = 0;          // Метка времени при записи (мс от начала)

    // Сериализация в JSON
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["type"]      = static_cast<int>(type);
        obj["vkCode"]    = vkCode;
        obj["delayMs"]   = delayMs;
        obj["timestamp"] = timestamp;
        return obj;
    }

    static KeyboardMacroAction fromJson(const QJsonObject& obj) {
        KeyboardMacroAction action;
        action.type      = static_cast<Type>(obj["type"].toInt());
        action.vkCode    = obj["vkCode"].toInt();
        action.delayMs   = obj["delayMs"].toInt();
        action.timestamp = obj["timestamp"].toInteger();
        return action;
    }
};

// Структура конфигурации автонажатия
struct KeyboardClickerConfig
{
    KeyboardMode mode          = KeyboardMode::SingleKey;

    int  virtualKeyCode        = 0;        // VK-код клавиши
    int  intervalMs            = 50;       // Интервал между нажатиями

    // Модификаторы
    bool withCtrl              = false;
    bool withShift             = false;
    bool withAlt               = false;
    bool withWin               = false;

    // Режим ввода текста
    QString textToType;                    // Текст для посимвольного ввода
    int  typeDelayMs           = 30;       // Задержка между символами

    // Ограничения
    int  repeatCount           = 0;        // 0 = бесконечно
    int  timeLimitMs           = 0;        // 0 = без ограничения

    // Рандомизация
    bool randomizeInterval     = false;
    int  randomIntervalMin     = 20;
    int  randomIntervalMax     = 100;

    // Записанный макрос (для режима MacroPlayback)
    QList<KeyboardMacroAction> macroActions;
};
