// ===========================================
// HotkeyManager.cpp — Реализация глобальных хоткеев
// ===========================================

#include "HotkeyManager.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
    // Устанавливаем фильтр нативных событий для перехвата WM_HOTKEY
    QCoreApplication::instance()->installNativeEventFilter(this);

    LOG_DEBUG(tr("Менеджер глобальных хоткеев инициализирован"));
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool HotkeyManager::registerHotkey(int id, UINT modifiers, UINT vk)
{
#ifdef Q_OS_WIN
    // Удаляем старый хоткей с этим ID, если есть
    if (m_registered.contains(id)) {
        unregisterHotkey(id);
    }

    // Регистрируем через Win32 API
    if (RegisterHotKey(nullptr, id, modifiers | MOD_NOREPEAT, vk)) {
        m_registered.insert(id, {modifiers, vk});
        LOG_INFO(tr("Хоткей зарегистрирован: ID=%1, VK=0x%2")
                     .arg(id)
                     .arg(vk, 2, 16, QChar('0')));
        return true;
    } else {
        LOG_ERROR(tr("Не удалось зарегистрировать хоткей: ID=%1 (возможно, занят)")
                      .arg(id));
        return false;
    }
#else
    Q_UNUSED(id); Q_UNUSED(modifiers); Q_UNUSED(vk);
    return false;
#endif
}

bool HotkeyManager::unregisterHotkey(int id)
{
#ifdef Q_OS_WIN
    if (UnregisterHotKey(nullptr, id)) {
        m_registered.remove(id);
        return true;
    }
    return false;
#else
    Q_UNUSED(id);
    return false;
#endif
}

void HotkeyManager::unregisterAll()
{
#ifdef Q_OS_WIN
    for (auto it = m_registered.begin(); it != m_registered.end(); ++it) {
        UnregisterHotKey(nullptr, it.key());
    }
    m_registered.clear();
#endif
}

void HotkeyManager::registerDefaults()
{
    using namespace AppConstants::Hotkeys;

    // F6 — Старт/Стоп автоклика
    registerHotkey(ID_START_STOP, 0, VK_F6);

    // F7 — Запись макроса
    registerHotkey(ID_RECORD_MACRO, 0, VK_F7);

    // F8 — Экстренная остановка всего
    registerHotkey(ID_EMERGENCY_STOP, 0, VK_F8);
}

bool HotkeyManager::nativeEventFilter(const QByteArray& eventType,
                                        void* message, qintptr* result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);

            emit hotkeyPressed(id);

            // Отправляем конкретные сигналы
            using namespace AppConstants::Hotkeys;
            if (id == ID_START_STOP) {
                emit startStopTriggered();
                LOG_DEBUG(tr("Хоткей: Старт/Стоп (F6)"));
            } else if (id == ID_RECORD_MACRO) {
                emit recordMacroTriggered();
                LOG_DEBUG(tr("Хоткей: Запись макроса (F7)"));
            } else if (id == ID_EMERGENCY_STOP) {
                emit emergencyStopTriggered();
                LOG_DEBUG(tr("Хоткей: Экстренная остановка (F8)"));
            }

            return true;  // Событие обработано
        }
    }
#else
    Q_UNUSED(eventType); Q_UNUSED(message);
#endif

    return false;
}
