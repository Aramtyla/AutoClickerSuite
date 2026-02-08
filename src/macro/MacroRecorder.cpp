// ===========================================
// MacroRecorder.cpp — Запись макросов в реальном времени
// через Win32 low-level hooks (мышь + клавиатура)
// ===========================================

#include "MacroRecorder.h"
#include "utils/Logger.h"
#include "utils/Constants.h"
#include "keyboard/KeyboardClicker.h"

#include <QtMath>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// Статические поля для hook callbacks
HHOOK          MacroRecorder::s_mouseHook    = nullptr;
HHOOK          MacroRecorder::s_keyboardHook = nullptr;
MacroRecorder* MacroRecorder::s_instance     = nullptr;

MacroRecorder::MacroRecorder(QObject* parent)
    : QObject(parent)
{
}

MacroRecorder::~MacroRecorder()
{
    if (m_recording) {
        stopRecording();
    }
}

// ===========================================
// Управление записью
// ===========================================

void MacroRecorder::startRecording()
{
    if (m_recording) return;

    m_actions.clear();
    m_recording    = true;
    m_paused       = false;
    m_pauseOffset  = 0;
    m_pauseStart   = 0;
    m_timer.start();

    // Запоминаем начальную позицию мыши
#ifdef Q_OS_WIN
    POINT pt;
    if (GetCursorPos(&pt)) {
        m_lastMousePos = QPoint(pt.x, pt.y);
    }
#endif

    installHooks();

    LOG_INFO(tr("Запись макроса начата (мышь: %1, клавиатура: %2, перемещения: %3)")
                 .arg(m_recordMouse ? tr("да") : tr("нет"))
                 .arg(m_recordKeyboard ? tr("да") : tr("нет"))
                 .arg(m_recordMouseMove ? tr("да") : tr("нет")));
    emit recordingStarted();
}

void MacroRecorder::stopRecording()
{
    if (!m_recording) return;

    m_recording = false;
    m_paused    = false;

    removeHooks();

    LOG_INFO(tr("Запись макроса завершена (действий: %1)").arg(m_actions.size()));
    emit recordingStopped();
}

void MacroRecorder::pauseRecording()
{
    if (!m_recording || m_paused) return;

    m_paused     = true;
    m_pauseStart = m_timer.elapsed();

    LOG_INFO(tr("Запись макроса приостановлена"));
    emit recordingPaused();
}

void MacroRecorder::resumeRecording()
{
    if (!m_recording || !m_paused) return;

    m_paused = false;
    // Добавляем время паузы к смещению
    m_pauseOffset += (m_timer.elapsed() - m_pauseStart);

    LOG_INFO(tr("Запись макроса возобновлена"));
    emit recordingResumed();
}

bool MacroRecorder::isRecording() const
{
    return m_recording;
}

bool MacroRecorder::isPaused() const
{
    return m_paused;
}

// ===========================================
// Настройки записи
// ===========================================

void MacroRecorder::setRecordMouse(bool enabled)
{
    m_recordMouse = enabled;
}

void MacroRecorder::setRecordKeyboard(bool enabled)
{
    m_recordKeyboard = enabled;
}

void MacroRecorder::setRecordMouseMove(bool enabled)
{
    m_recordMouseMove = enabled;
}

void MacroRecorder::setMinMoveDistance(int pixels)
{
    m_minMoveDistance = qMax(1, pixels);
}

bool MacroRecorder::recordMouse() const { return m_recordMouse; }
bool MacroRecorder::recordKeyboard() const { return m_recordKeyboard; }
bool MacroRecorder::recordMouseMove() const { return m_recordMouseMove; }

// ===========================================
// Результат
// ===========================================

QList<MacroAction> MacroRecorder::recordedActions() const
{
    return m_actions;
}

void MacroRecorder::clearRecordedActions()
{
    m_actions.clear();
}

int MacroRecorder::actionCount() const
{
    return m_actions.size();
}

// ===========================================
// Добавление действия с автоматической задержкой
// ===========================================

void MacroRecorder::appendAction(const MacroAction& action)
{
    // Проверяем лимит действий
    if (m_actions.size() >= AppConstants::Macro::MAX_ACTIONS) {
        LOG_WARNING(tr("Достигнут лимит действий макроса: %1")
                        .arg(AppConstants::Macro::MAX_ACTIONS));
        stopRecording();
        return;
    }

    // Вычисляем реальную метку времени (с учётом пауз)
    qint64 currentTime = m_timer.elapsed() - m_pauseOffset;

    // Вставляем задержку между действиями (если это не первое)
    if (!m_actions.isEmpty()) {
        qint64 prevTime = m_actions.last().timestamp;
        int delay = static_cast<int>(currentTime - prevTime);
        if (delay > 0) {
            MacroAction delayAction;
            delayAction.type      = MacroActionType::Delay;
            delayAction.delayMs   = delay;
            delayAction.timestamp = prevTime;
            m_actions.append(delayAction);
            emit actionRecorded(delayAction);
        }
    }

    // Добавляем основное действие с меткой времени
    MacroAction act = action;
    act.timestamp = currentTime;
    m_actions.append(act);
    emit actionRecorded(act);
}

// ===========================================
// Проверка значимости перемещения
// ===========================================

bool MacroRecorder::isSignificantMove(const QPoint& newPos) const
{
    int dx = newPos.x() - m_lastMousePos.x();
    int dy = newPos.y() - m_lastMousePos.y();
    double dist = qSqrt(static_cast<double>(dx * dx + dy * dy));
    return dist >= m_minMoveDistance;
}

// ===========================================
// Win32 Hooks: установка / снятие
// ===========================================

void MacroRecorder::installHooks()
{
#ifdef Q_OS_WIN
    s_instance = this;

    // Хук мыши
    if (m_recordMouse) {
        s_mouseHook = SetWindowsHookEx(
            WH_MOUSE_LL,
            mouseHookProc,
            GetModuleHandle(nullptr),
            0
        );
        if (!s_mouseHook) {
            LOG_ERROR(tr("Не удалось установить mouse hook (ошибка: %1)")
                          .arg(GetLastError()));
            emit error(tr("Не удалось установить перехват мыши"));
        }
    }

    // Хук клавиатуры
    if (m_recordKeyboard) {
        s_keyboardHook = SetWindowsHookEx(
            WH_KEYBOARD_LL,
            keyboardHookProc,
            GetModuleHandle(nullptr),
            0
        );
        if (!s_keyboardHook) {
            LOG_ERROR(tr("Не удалось установить keyboard hook (ошибка: %1)")
                          .arg(GetLastError()));
            emit error(tr("Не удалось установить перехват клавиатуры"));
        }
    }
#endif
}

void MacroRecorder::removeHooks()
{
#ifdef Q_OS_WIN
    if (s_mouseHook) {
        UnhookWindowsHookEx(s_mouseHook);
        s_mouseHook = nullptr;
    }
    if (s_keyboardHook) {
        UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = nullptr;
    }
    s_instance = nullptr;
#endif
}

// ===========================================
// Win32 Mouse Hook Callback
// ===========================================

LRESULT CALLBACK MacroRecorder::mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifdef Q_OS_WIN
    if (nCode == HC_ACTION && s_instance && s_instance->m_recording && !s_instance->m_paused) {
        MSLLHOOKSTRUCT* mhs = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

        // Игнорируем инъецированные события
        if (mhs->flags & LLMHF_INJECTED) {
            return CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
        }

        MacroAction action;
        action.position    = QPoint(static_cast<int>(mhs->pt.x),
                                     static_cast<int>(mhs->pt.y));
        action.useCurrentPos = false;

        bool shouldRecord = true;

        switch (wParam) {
            case WM_LBUTTONDOWN:
                action.type        = MacroActionType::MouseDown;
                action.mouseButton = MacroMouseButton::Left;
                break;
            case WM_LBUTTONUP:
                action.type        = MacroActionType::MouseUp;
                action.mouseButton = MacroMouseButton::Left;
                break;
            case WM_RBUTTONDOWN:
                action.type        = MacroActionType::MouseDown;
                action.mouseButton = MacroMouseButton::Right;
                break;
            case WM_RBUTTONUP:
                action.type        = MacroActionType::MouseUp;
                action.mouseButton = MacroMouseButton::Right;
                break;
            case WM_MBUTTONDOWN:
                action.type        = MacroActionType::MouseDown;
                action.mouseButton = MacroMouseButton::Middle;
                break;
            case WM_MBUTTONUP:
                action.type        = MacroActionType::MouseUp;
                action.mouseButton = MacroMouseButton::Middle;
                break;
            case WM_MOUSEMOVE:
                if (s_instance->m_recordMouseMove) {
                    // Фильтруем незначительные перемещения
                    QPoint newPos(static_cast<int>(mhs->pt.x),
                                  static_cast<int>(mhs->pt.y));
                    if (s_instance->isSignificantMove(newPos)) {
                        action.type = MacroActionType::MouseMove;
                        s_instance->m_lastMousePos = newPos;
                    } else {
                        shouldRecord = false;
                    }
                } else {
                    shouldRecord = false;
                }
                break;
            default:
                shouldRecord = false;
                break;
        }

        if (shouldRecord) {
            // Обновляем позицию для кликов
            if (action.type != MacroActionType::MouseMove) {
                s_instance->m_lastMousePos = action.position;
            }
            s_instance->appendAction(action);
        }
    }

    return CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
#else
    Q_UNUSED(nCode); Q_UNUSED(wParam); Q_UNUSED(lParam);
    return 0;
#endif
}

// ===========================================
// Win32 Keyboard Hook Callback
// ===========================================

LRESULT CALLBACK MacroRecorder::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifdef Q_OS_WIN
    if (nCode == HC_ACTION && s_instance && s_instance->m_recording && !s_instance->m_paused) {
        KBDLLHOOKSTRUCT* kbs = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        // Игнорируем инъецированные события
        if (kbs->flags & LLKHF_INJECTED) {
            return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
        }

        // Игнорируем хоткеи приложения (F6, F7, F8)
        WORD vk = static_cast<WORD>(kbs->vkCode);
        if (vk == VK_F6 || vk == VK_F7 || vk == VK_F8) {
            return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
        }

        MacroAction action;
        action.vkCode = static_cast<int>(kbs->vkCode);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            action.type = MacroActionType::KeyDown;
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            action.type = MacroActionType::KeyUp;
        } else {
            return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
        }

        s_instance->appendAction(action);
    }

    return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
#else
    Q_UNUSED(nCode); Q_UNUSED(wParam); Q_UNUSED(lParam);
    return 0;
#endif
}
