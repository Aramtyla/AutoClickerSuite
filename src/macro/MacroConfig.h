#pragma once
// ===========================================
// MacroConfig.h — Структуры данных макро-движка
// Поддерживает смешанные действия: мышь + клавиатура +
// задержки + циклы + вложенные макросы
// ===========================================

#include <QString>
#include <QList>
#include <QPoint>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// ==========================================
// Тип действия макроса
// ==========================================
enum class MacroActionType {
    // Мышь
    MouseClick,          // Клик мышью (кнопка, позиция, тип клика)
    MouseDoubleClick,    // Двойной клик
    MouseDown,           // Нажать кнопку мыши (без отпускания)
    MouseUp,             // Отпустить кнопку мыши
    MouseMove,           // Переместить курсор

    // Клавиатура
    KeyDown,             // Нажать клавишу
    KeyUp,               // Отпустить клавишу
    KeyPress,            // Нажать и отпустить клавишу
    KeyCombo,            // Комбинация клавиш (модификаторы + клавиша)
    TypeText,            // Ввести строку текста

    // Управление потоком
    Delay,               // Фиксированная задержка
    RandomDelay,         // Случайная задержка в диапазоне
    LoopStart,           // Начало цикла
    LoopEnd,             // Конец цикла

    // Вложенный макрос
    SubMacro,            // Выполнить другой макрос по имени/пути

    // Комментарий (для визуального редактора)
    Comment
};

// ==========================================
// Кнопка мыши (для действий макроса)
// ==========================================
enum class MacroMouseButton {
    Left   = 0,
    Right  = 1,
    Middle = 2
};

// ==========================================
// Одно действие макроса
// ==========================================
struct MacroAction
{
    MacroActionType  type       = MacroActionType::Delay;

    // === Параметры мыши ===
    MacroMouseButton mouseButton = MacroMouseButton::Left;
    QPoint           position    = {0, 0};    // Координаты для MouseMove / MouseClick
    bool             useCurrentPos = true;    // Использовать текущую позицию курсора

    // === Параметры клавиатуры ===
    int              vkCode     = 0;          // VK-код клавиши
    bool             withCtrl   = false;      // Модификаторы для KeyCombo
    bool             withShift  = false;
    bool             withAlt    = false;
    bool             withWin    = false;
    QString          text;                    // Текст для TypeText

    // === Параметры задержки ===
    int              delayMs    = 100;        // Задержка (мс)
    int              delayMinMs = 50;         // Мин. для RandomDelay
    int              delayMaxMs = 200;        // Макс. для RandomDelay

    // === Параметры цикла ===
    int              loopCount  = 1;          // Количество повторений (0 = бесконечно)

    // === Вложенный макрос ===
    QString          subMacroName;            // Имя вложенного макроса

    // === Комментарий ===
    QString          comment;

    // === Метка времени (при записи) ===
    qint64           timestamp  = 0;          // Мс от начала записи

    // ==========================================
    // Сериализация в JSON
    // ==========================================
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["type"]          = static_cast<int>(type);
        obj["mouseButton"]   = static_cast<int>(mouseButton);
        obj["posX"]          = position.x();
        obj["posY"]          = position.y();
        obj["useCurrentPos"] = useCurrentPos;
        obj["vkCode"]        = vkCode;
        obj["withCtrl"]      = withCtrl;
        obj["withShift"]     = withShift;
        obj["withAlt"]       = withAlt;
        obj["withWin"]       = withWin;
        obj["text"]          = text;
        obj["delayMs"]       = delayMs;
        obj["delayMinMs"]    = delayMinMs;
        obj["delayMaxMs"]    = delayMaxMs;
        obj["loopCount"]     = loopCount;
        obj["subMacroName"]  = subMacroName;
        obj["comment"]       = comment;
        obj["timestamp"]     = timestamp;
        return obj;
    }

    static MacroAction fromJson(const QJsonObject& obj) {
        MacroAction a;
        a.type          = static_cast<MacroActionType>(obj["type"].toInt());
        a.mouseButton   = static_cast<MacroMouseButton>(obj["mouseButton"].toInt());
        a.position      = QPoint(obj["posX"].toInt(), obj["posY"].toInt());
        a.useCurrentPos = obj["useCurrentPos"].toBool(true);
        a.vkCode        = obj["vkCode"].toInt();
        a.withCtrl      = obj["withCtrl"].toBool();
        a.withShift     = obj["withShift"].toBool();
        a.withAlt       = obj["withAlt"].toBool();
        a.withWin       = obj["withWin"].toBool();
        a.text          = obj["text"].toString();
        a.delayMs       = obj["delayMs"].toInt(100);
        a.delayMinMs    = obj["delayMinMs"].toInt(50);
        a.delayMaxMs    = obj["delayMaxMs"].toInt(200);
        a.loopCount     = obj["loopCount"].toInt(1);
        a.subMacroName  = obj["subMacroName"].toString();
        a.comment       = obj["comment"].toString();
        a.timestamp     = obj["timestamp"].toInteger();
        return a;
    }

    // ==========================================
    // Описание для GUI
    // ==========================================
    QString description() const;
};

// ==========================================
// Макрос — набор действий с метаданными
// ==========================================
struct Macro
{
    QString              name;                 // Имя макроса
    QString              description;          // Описание
    QList<MacroAction>   actions;              // Список действий
    int                  repeatCount  = 1;     // Сколько раз повторить (0 = бесконечно)
    double               speedMultiplier = 1.0; // Множитель скорости воспроизведения
    QString              createdAt;            // Дата создания
    QString              modifiedAt;           // Дата изменения

    // ==========================================
    // Сериализация в JSON
    // ==========================================
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"]            = name;
        obj["description"]     = description;
        obj["repeatCount"]     = repeatCount;
        obj["speedMultiplier"] = speedMultiplier;
        obj["createdAt"]       = createdAt;
        obj["modifiedAt"]      = modifiedAt;

        QJsonArray arr;
        for (const auto& action : actions) {
            arr.append(action.toJson());
        }
        obj["actions"] = arr;
        return obj;
    }

    static Macro fromJson(const QJsonObject& obj) {
        Macro m;
        m.name            = obj["name"].toString();
        m.description     = obj["description"].toString();
        m.repeatCount     = obj["repeatCount"].toInt(1);
        m.speedMultiplier = obj["speedMultiplier"].toDouble(1.0);
        m.createdAt       = obj["createdAt"].toString();
        m.modifiedAt      = obj["modifiedAt"].toString();

        QJsonArray arr = obj["actions"].toArray();
        for (const auto& val : arr) {
            m.actions.append(MacroAction::fromJson(val.toObject()));
        }
        return m;
    }

    // Сериализация полного документа
    QByteArray toJsonBytes() const {
        QJsonDocument doc(toJson());
        return doc.toJson(QJsonDocument::Indented);
    }

    static Macro fromJsonBytes(const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        return fromJson(doc.object());
    }

    bool isValid() const {
        return !name.isEmpty() && !actions.isEmpty();
    }
};
