#pragma once
// ===========================================
// HotkeyManager.h — Глобальные хоткеи (Win32 RegisterHotKey)
// ===========================================

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QMap>
#include <functional>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager();

    // Регистрация глобального хоткея
    // id — уникальный идентификатор (из Constants.h)
    // modifiers — MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN (Win32)
    // vk — виртуальный код клавиши (VK_F6, VK_F7 и т.д.)
    bool registerHotkey(int id, UINT modifiers, UINT vk);

    // Удаление хоткея
    bool unregisterHotkey(int id);

    // Удаление всех хоткеев
    void unregisterAll();

    // Регистрация хоткеев по умолчанию
    void registerDefaults();

    // Перехват нативных событий Windows
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message, qintptr* result) override;

signals:
    // Сигналы при нажатии глобальных хоткеев
    void hotkeyPressed(int id);
    void startStopTriggered();
    void recordMacroTriggered();
    void emergencyStopTriggered();
    void exitAppTriggered();

private:
    QMap<int, QPair<UINT, UINT>> m_registered;  // id -> (modifiers, vk)
    HWND m_hwnd = nullptr;
};
