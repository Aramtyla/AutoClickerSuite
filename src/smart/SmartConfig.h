#pragma once
// ===========================================
// SmartConfig.h — Конфигурации умных режимов
// Привязка к окну, клик по цвету, по изображению,
// планировщик
// ===========================================

#include <QString>
#include <QColor>
#include <QPoint>
#include <QRect>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

// ==========================================
// Конфигурация привязки к окну
// ==========================================
struct WindowBindingConfig
{
    bool     enabled       = false;    // Включена ли привязка
    QString  windowTitle;              // Заголовок окна (частичное совпадение)
    QString  windowClass;              // Класс окна (точное совпадение)
    quintptr windowHandle  = 0;        // HWND (выбранное окно)
    bool     bringToFront  = false;    // Выводить окно на передний план перед действием
    bool     useClientArea = true;     // Координаты относительно клиентской области

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["enabled"]       = enabled;
        obj["windowTitle"]   = windowTitle;
        obj["windowClass"]   = windowClass;
        obj["bringToFront"]  = bringToFront;
        obj["useClientArea"] = useClientArea;
        return obj;
    }

    static WindowBindingConfig fromJson(const QJsonObject& obj) {
        WindowBindingConfig c;
        c.enabled       = obj["enabled"].toBool();
        c.windowTitle   = obj["windowTitle"].toString();
        c.windowClass   = obj["windowClass"].toString();
        c.bringToFront  = obj["bringToFront"].toBool();
        c.useClientArea = obj["useClientArea"].toBool(true);
        return c;
    }
};

// ==========================================
// Конфигурация клика по цвету пикселя
// ==========================================
struct ColorMatchConfig
{
    bool     enabled       = false;    // Включён ли режим
    QColor   targetColor   = Qt::red;  // Целевой цвет
    int      tolerance     = 10;       // Допуск отклонения (0-255)
    QRect    searchArea;               // Область поиска (пустой = весь экран)
    bool     searchFullScreen = true;  // Искать по всему экрану
    int      scanIntervalMs = 100;     // Интервал сканирования (мс)

    // Действие при обнаружении
    enum class Action {
        Click,          // Клик по найденной позиции
        DoubleClick,    // Двойной клик
        MoveTo,         // Переместить курсор
        Notify          // Только уведомить
    };
    Action   action = Action::Click;

    // Ожидать исчезновения цвета перед повторным срабатыванием
    bool     waitForDisappear = true;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["enabled"]          = enabled;
        obj["colorR"]           = targetColor.red();
        obj["colorG"]           = targetColor.green();
        obj["colorB"]           = targetColor.blue();
        obj["tolerance"]        = tolerance;
        obj["searchFullScreen"] = searchFullScreen;
        obj["searchX"]          = searchArea.x();
        obj["searchY"]          = searchArea.y();
        obj["searchW"]          = searchArea.width();
        obj["searchH"]          = searchArea.height();
        obj["scanIntervalMs"]   = scanIntervalMs;
        obj["action"]           = static_cast<int>(action);
        obj["waitForDisappear"] = waitForDisappear;
        return obj;
    }

    static ColorMatchConfig fromJson(const QJsonObject& obj) {
        ColorMatchConfig c;
        c.enabled          = obj["enabled"].toBool();
        c.targetColor      = QColor(obj["colorR"].toInt(255),
                                     obj["colorG"].toInt(0),
                                     obj["colorB"].toInt(0));
        c.tolerance        = obj["tolerance"].toInt(10);
        c.searchFullScreen = obj["searchFullScreen"].toBool(true);
        c.searchArea       = QRect(obj["searchX"].toInt(),
                                    obj["searchY"].toInt(),
                                    obj["searchW"].toInt(),
                                    obj["searchH"].toInt());
        c.scanIntervalMs   = obj["scanIntervalMs"].toInt(100);
        c.action           = static_cast<Action>(obj["action"].toInt());
        c.waitForDisappear = obj["waitForDisappear"].toBool(true);
        return c;
    }
};

// ==========================================
// Конфигурация клика по изображению
// (Template Matching)
// ==========================================
struct ImageMatchConfig
{
    bool     enabled          = false;    // Включён ли режим
    QString  templatePath;               // Путь к изображению-шаблону
    double   threshold        = 0.85;    // Порог совпадения (0.0-1.0)
    QRect    searchArea;                 // Область поиска
    bool     searchFullScreen = true;
    int      scanIntervalMs   = 500;     // Интервал сканирования (мс)

    // Действие при обнаружении
    enum class Action {
        ClickCenter,    // Клик по центру найденного фрагмента
        DoubleClick,    // Двойной клик
        MoveTo,         // Переместить курсор
        Notify          // Только уведомить
    };
    Action   action = Action::ClickCenter;

    // Ожидать исчезновения перед повторным срабатыванием
    bool     waitForDisappear = true;

    // Преобразовать в оттенки серого при сравнении
    bool     useGrayscale     = true;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["enabled"]          = enabled;
        obj["templatePath"]     = templatePath;
        obj["threshold"]        = threshold;
        obj["searchFullScreen"] = searchFullScreen;
        obj["searchX"]          = searchArea.x();
        obj["searchY"]          = searchArea.y();
        obj["searchW"]          = searchArea.width();
        obj["searchH"]          = searchArea.height();
        obj["scanIntervalMs"]   = scanIntervalMs;
        obj["action"]           = static_cast<int>(action);
        obj["waitForDisappear"] = waitForDisappear;
        obj["useGrayscale"]     = useGrayscale;
        return obj;
    }

    static ImageMatchConfig fromJson(const QJsonObject& obj) {
        ImageMatchConfig c;
        c.enabled          = obj["enabled"].toBool();
        c.templatePath     = obj["templatePath"].toString();
        c.threshold        = obj["threshold"].toDouble(0.85);
        c.searchFullScreen = obj["searchFullScreen"].toBool(true);
        c.searchArea       = QRect(obj["searchX"].toInt(),
                                    obj["searchY"].toInt(),
                                    obj["searchW"].toInt(),
                                    obj["searchH"].toInt());
        c.scanIntervalMs   = obj["scanIntervalMs"].toInt(500);
        c.action           = static_cast<Action>(obj["action"].toInt());
        c.waitForDisappear = obj["waitForDisappear"].toBool(true);
        c.useGrayscale     = obj["useGrayscale"].toBool(true);
        return c;
    }
};

// ==========================================
// Задание планировщика
// ==========================================
struct SchedulerTask
{
    QString    id;                        // Уникальный ID задания
    QString    name;                      // Название задания
    QString    macroName;                 // Имя макроса для выполнения
    QDateTime  scheduledTime;             // Дата/время запуска
    bool       repeat       = false;      // Повторять
    int        repeatIntervalMin = 60;    // Интервал повтора (минуты)
    bool       enabled      = true;       // Включено ли

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"]                = id;
        obj["name"]              = name;
        obj["macroName"]         = macroName;
        obj["scheduledTime"]     = scheduledTime.toString(Qt::ISODate);
        obj["repeat"]            = repeat;
        obj["repeatIntervalMin"] = repeatIntervalMin;
        obj["enabled"]           = enabled;
        return obj;
    }

    static SchedulerTask fromJson(const QJsonObject& obj) {
        SchedulerTask t;
        t.id                = obj["id"].toString();
        t.name              = obj["name"].toString();
        t.macroName         = obj["macroName"].toString();
        t.scheduledTime     = QDateTime::fromString(obj["scheduledTime"].toString(), Qt::ISODate);
        t.repeat            = obj["repeat"].toBool();
        t.repeatIntervalMin = obj["repeatIntervalMin"].toInt(60);
        t.enabled           = obj["enabled"].toBool(true);
        return t;
    }
};

