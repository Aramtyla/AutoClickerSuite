// ===========================================
// ProfileManager.cpp — Полная реализация профилей
// Сбор и применение настроек всех модулей:
// мышь, клавиатура, умные режимы
// ===========================================

#include "ProfileManager.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include "mouse/MouseWidget.h"
#include "mouse/MouseClicker.h"
#include "mouse/MouseConfig.h"

#include "keyboard/KeyboardWidget.h"
#include "keyboard/KeyboardClicker.h"
#include "keyboard/KeyboardConfig.h"

#include "smart/SmartWidget.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDateTime>

ProfileManager::ProfileManager(QObject* parent)
    : QObject(parent)
{
    // Создаём директорию профилей, если её нет
    QDir().mkpath(profilesDirectory());
    LOG_DEBUG(tr("Менеджер профилей инициализирован: %1").arg(profilesDirectory()));
}

void ProfileManager::setModuleWidgets(MouseWidget* mouse, KeyboardWidget* keyboard,
                                       SmartWidget* smart)
{
    m_mouseWidget    = mouse;
    m_keyboardWidget = keyboard;
    m_smartWidget    = smart;
}

bool ProfileManager::saveProfile(const QString& name)
{
    QJsonObject settings = collectSettings();
    QJsonDocument doc(settings);

    QString filePath = profilesDirectory() + "/" + name + ".json";
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(tr("Не удалось сохранить профиль: %1").arg(filePath));
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    m_currentProfile = name;

    LOG_INFO(tr("Профиль сохранён: %1").arg(name));
    emit profileSaved(name);
    emit profileListChanged();
    return true;
}

bool ProfileManager::loadProfile(const QString& name)
{
    QString filePath = profilesDirectory() + "/" + name + ".json";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(tr("Не удалось загрузить профиль: %1").arg(filePath));
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(tr("Ошибка разбора профиля: %1").arg(parseError.errorString()));
        return false;
    }

    applySettings(doc.object());
    m_currentProfile = name;

    LOG_INFO(tr("Профиль загружен: %1").arg(name));
    emit profileLoaded(name);
    return true;
}

bool ProfileManager::deleteProfile(const QString& name)
{
    QString filePath = profilesDirectory() + "/" + name + ".json";
    bool ok = QFile::remove(filePath);
    if (ok) {
        emit profileListChanged();
        LOG_INFO(tr("Профиль удалён: %1").arg(name));
    }
    return ok;
}

QStringList ProfileManager::availableProfiles() const
{
    QDir dir(profilesDirectory());
    QStringList filters = {"*.json"};
    QStringList files = dir.entryList(filters, QDir::Files);

    // Убираем расширение .json для отображения
    QStringList profiles;
    for (const QString& f : files) {
        profiles.append(f.chopped(5)); // убираем ".json"
    }
    return profiles;
}

QString ProfileManager::currentProfile() const
{
    return m_currentProfile;
}

QString ProfileManager::profilesDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/" + AppConstants::Paths::PROFILES_DIR;
}

// ==========================================
// Сбор настроек из всех модулей
// ==========================================

QJsonObject ProfileManager::collectSettings() const
{
    QJsonObject obj;
    obj["version"]   = APP_VERSION;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // === Мышь ===
    if (m_mouseWidget) {
        auto cfg = m_mouseWidget->clicker()->config();
        QJsonObject mouseObj;
        mouseObj["button"]             = static_cast<int>(cfg.button);
        mouseObj["clickType"]          = static_cast<int>(cfg.clickType);
        mouseObj["positionMode"]       = static_cast<int>(cfg.positionMode);
        mouseObj["intervalMs"]         = cfg.intervalMs;
        mouseObj["clickCount"]         = cfg.clickCount;
        mouseObj["holdDurationMs"]     = cfg.holdDurationMs;
        mouseObj["fixedPosX"]          = cfg.fixedPos.x();
        mouseObj["fixedPosY"]          = cfg.fixedPos.y();
        mouseObj["randomizeInterval"]  = cfg.randomizeInterval;
        mouseObj["randomIntervalMin"]  = cfg.randomIntervalMin;
        mouseObj["randomIntervalMax"]  = cfg.randomIntervalMax;
        mouseObj["randomizePosition"]  = cfg.randomizePosition;
        mouseObj["randomOffsetPixels"] = cfg.randomOffsetPixels;
        mouseObj["timeLimitMs"]        = cfg.timeLimitMs;

        // Мультиточечный маршрут
        QJsonArray pointsArr;
        auto points = m_mouseWidget->clicker()->clickPoints();
        for (const auto& pt : points) {
            QJsonObject ptObj;
            ptObj["x"] = pt.x();
            ptObj["y"] = pt.y();
            pointsArr.append(ptObj);
        }
        mouseObj["multiPoints"] = pointsArr;

        obj["mouse"] = mouseObj;
    }

    // === Клавиатура ===
    if (m_keyboardWidget) {
        auto cfg = m_keyboardWidget->clicker()->config();
        QJsonObject keyObj;
        keyObj["mode"]              = static_cast<int>(cfg.mode);
        keyObj["vkCode"]            = cfg.virtualKeyCode;
        keyObj["withCtrl"]          = cfg.withCtrl;
        keyObj["withShift"]         = cfg.withShift;
        keyObj["withAlt"]           = cfg.withAlt;
        keyObj["withWin"]           = cfg.withWin;
        keyObj["intervalMs"]        = cfg.intervalMs;
        keyObj["text"]              = cfg.textToType;
        keyObj["typeDelayMs"]       = cfg.typeDelayMs;
        keyObj["repeatCount"]       = cfg.repeatCount;
        keyObj["timeLimitMs"]       = cfg.timeLimitMs;
        keyObj["randomizeInterval"] = cfg.randomizeInterval;
        keyObj["randomIntervalMin"] = cfg.randomIntervalMin;
        keyObj["randomIntervalMax"] = cfg.randomIntervalMax;

        // Записанные действия макроса клавиатуры
        QJsonArray macroArr;
        auto actions = m_keyboardWidget->clicker()->recordedActions();
        for (const auto& act : actions) {
            macroArr.append(act.toJson());
        }
        keyObj["macroActions"] = macroArr;

        obj["keyboard"] = keyObj;
    }

    // === Умные режимы ===
    if (m_smartWidget) {
        obj["smart"] = m_smartWidget->collectSettings();
    }

    return obj;
}

// ==========================================
// Применение настроек ко всем модулям
// ==========================================

void ProfileManager::applySettings(const QJsonObject& settings)
{
    // === Мышь ===
    if (settings.contains("mouse") && m_mouseWidget) {
        QJsonObject mouseObj = settings["mouse"].toObject();
        MouseClickerConfig cfg;
        cfg.button             = static_cast<MouseButton>(mouseObj["button"].toInt());
        cfg.clickType          = static_cast<ClickType>(mouseObj["clickType"].toInt());
        cfg.positionMode       = static_cast<ClickPosition>(mouseObj["positionMode"].toInt());
        cfg.intervalMs         = mouseObj["intervalMs"].toInt(100);
        cfg.clickCount         = mouseObj["clickCount"].toInt(0);
        cfg.holdDurationMs     = mouseObj["holdDurationMs"].toInt(100);
        cfg.fixedPos           = QPoint(mouseObj["fixedPosX"].toInt(),
                                         mouseObj["fixedPosY"].toInt());
        cfg.randomizeInterval  = mouseObj["randomizeInterval"].toBool();
        cfg.randomIntervalMin  = mouseObj["randomIntervalMin"].toInt(50);
        cfg.randomIntervalMax  = mouseObj["randomIntervalMax"].toInt(200);
        cfg.randomizePosition  = mouseObj["randomizePosition"].toBool();
        cfg.randomOffsetPixels = mouseObj["randomOffsetPixels"].toInt(5);
        cfg.timeLimitMs        = mouseObj["timeLimitMs"].toInt(0);
        m_mouseWidget->clicker()->setConfig(cfg);

        // Мультиточечный маршрут
        QJsonArray pointsArr = mouseObj["multiPoints"].toArray();
        QVector<QPoint> points;
        for (const auto& val : pointsArr) {
            QJsonObject ptObj = val.toObject();
            points.append(QPoint(ptObj["x"].toInt(), ptObj["y"].toInt()));
        }
        m_mouseWidget->clicker()->setClickPoints(points);
    }

    // === Клавиатура ===
    if (settings.contains("keyboard") && m_keyboardWidget) {
        QJsonObject keyObj = settings["keyboard"].toObject();
        KeyboardClickerConfig cfg;
        cfg.mode              = static_cast<KeyboardMode>(keyObj["mode"].toInt());
        cfg.virtualKeyCode    = keyObj["vkCode"].toInt();
        cfg.withCtrl          = keyObj["withCtrl"].toBool();
        cfg.withShift         = keyObj["withShift"].toBool();
        cfg.withAlt           = keyObj["withAlt"].toBool();
        cfg.withWin           = keyObj["withWin"].toBool();
        cfg.intervalMs        = keyObj["intervalMs"].toInt(50);
        cfg.textToType        = keyObj["text"].toString();
        cfg.typeDelayMs       = keyObj["typeDelayMs"].toInt(30);
        cfg.repeatCount       = keyObj["repeatCount"].toInt(0);
        cfg.timeLimitMs       = keyObj["timeLimitMs"].toInt(0);
        cfg.randomizeInterval = keyObj["randomizeInterval"].toBool();
        cfg.randomIntervalMin = keyObj["randomIntervalMin"].toInt(20);
        cfg.randomIntervalMax = keyObj["randomIntervalMax"].toInt(100);
        m_keyboardWidget->clicker()->setConfig(cfg);

        // Макрос клавиатуры
        QJsonArray macroArr = keyObj["macroActions"].toArray();
        QList<KeyboardMacroAction> actions;
        for (const auto& val : macroArr) {
            actions.append(KeyboardMacroAction::fromJson(val.toObject()));
        }
        m_keyboardWidget->clicker()->setRecordedActions(actions);
    }

    // === Умные режимы ===
    if (settings.contains("smart") && m_smartWidget) {
        m_smartWidget->applySettings(settings["smart"].toObject());
    }

    LOG_INFO(tr("Настройки профиля применены ко всем модулям"));
}
