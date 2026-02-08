// ===========================================
// KeyboardClicker.cpp — Реализация движка автонажатия
// ===========================================

#include "KeyboardClicker.h"
#include "core/InputSimulator.h"
#include "utils/Logger.h"

#include <QThread>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// Статические поля для Win32 hook callback
HHOOK            KeyboardClicker::s_keyboardHook = nullptr;
KeyboardClicker* KeyboardClicker::s_instance     = nullptr;

KeyboardClicker::KeyboardClicker(QObject* parent)
    : QObject(parent)
{
    m_input = new InputSimulator(this);

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
}

KeyboardClicker::~KeyboardClicker()
{
    stop();
}

// ===========================================
// Управление
// ===========================================

void KeyboardClicker::start()
{
    if (m_running) return;

    m_running    = true;
    m_pressCount = 0;
    m_textIndex  = 0;
    m_macroIndex = 0;
    m_elapsed.start();

    switch (m_config.mode) {
        case KeyboardMode::SingleKey:
        case KeyboardMode::KeyCombination:
            // Обычный режим: тикаем по интервалу
            disconnect(m_timer, &QTimer::timeout, nullptr, nullptr);
            connect(m_timer, &QTimer::timeout, this, &KeyboardClicker::onTimerTick);
            m_timer->start(nextInterval());
            break;

        case KeyboardMode::HoldKey: {
            // Режим зажатия: нажимаем клавишу и держим до остановки
            disconnect(m_timer, &QTimer::timeout, nullptr, nullptr);
            int vk = m_config.virtualKeyCode;
            QList<quint16> mods = buildModifiers();
            for (quint16 mod : mods) {
                m_input->keyDown(mod);
            }
            m_input->keyDown(static_cast<WORD>(vk));
            m_pressCount = 1;
            emit keyPressed(1, vk);
            // Таймер для проверки лимитов
            if (m_config.timeLimitMs > 0) {
                connect(m_timer, &QTimer::timeout, this, [this]() {
                    if (checkLimits()) {
                        stop();
                        emit finished();
                    }
                });
                m_timer->start(100); // Проверяем каждые 100мс
            }
            break;
        }

        case KeyboardMode::TypeText:
            // Режим печати текста: один символ за тик
            disconnect(m_timer, &QTimer::timeout, nullptr, nullptr);
            connect(m_timer, &QTimer::timeout, this, &KeyboardClicker::onTypeNextChar);
            m_timer->start(m_config.typeDelayMs);
            break;

        case KeyboardMode::MacroPlayback:
            // Воспроизведение макроса: задержки из записанных данных
            disconnect(m_timer, &QTimer::timeout, nullptr, nullptr);
            connect(m_timer, &QTimer::timeout, this, &KeyboardClicker::onPlayNextAction);
            if (!m_config.macroActions.isEmpty()) {
                // Первое действие — сразу
                m_timer->start(0);
            } else {
                LOG_WARNING(tr("Макросный режим: нет записанных действий"));
                m_running = false;
                emit error(tr("Нет записанных действий для воспроизведения"));
                return;
            }
            break;
    }

    QString modeStr;
    switch (m_config.mode) {
        case KeyboardMode::SingleKey:      modeStr = tr("одиночная клавиша"); break;
        case KeyboardMode::KeyCombination: modeStr = tr("комбинация клавиш"); break;
        case KeyboardMode::HoldKey:        modeStr = tr("зажатие клавиши"); break;
        case KeyboardMode::TypeText:       modeStr = tr("ввод текста"); break;
        case KeyboardMode::MacroPlayback:  modeStr = tr("воспроизведение макроса"); break;
    }

    LOG_INFO(tr("Автонажатие клавиатуры запущено (режим: %1)").arg(modeStr));
    emit started();
}

void KeyboardClicker::stop()
{
    if (!m_running) return;

    m_timer->stop();

    // Если в режиме зажатия — отпускаем клавишу
    if (m_config.mode == KeyboardMode::HoldKey) {
        int vk = m_config.virtualKeyCode;
        m_input->keyUp(static_cast<WORD>(vk));
        QList<quint16> mods = buildModifiers();
        for (int i = mods.size() - 1; i >= 0; --i) {
            m_input->keyUp(mods[i]);
        }
    }

    m_running = false;

    qint64 elapsed = m_elapsed.elapsed();
    LOG_INFO(tr("Автонажатие клавиатуры остановлено (нажатий: %1, время: %2 мс)")
                 .arg(m_pressCount).arg(elapsed));

    emit statsUpdated(m_pressCount, elapsed);
    emit stopped();
}

void KeyboardClicker::toggleStartStop()
{
    if (m_running) {
        stop();
    } else {
        start();
    }
}

bool KeyboardClicker::isRunning() const
{
    return m_running;
}

// ===========================================
// Запись макроса
// ===========================================

void KeyboardClicker::startRecording()
{
    if (m_recording) return;

    m_recordedActions.clear();
    m_recording = true;
    m_recordTimer.start();

    // Устанавливаем глобальный перехват клавиатуры
    installKeyboardHook();

    LOG_INFO(tr("Запись клавиатурного макроса начата"));
    emit recordingStarted();
}

void KeyboardClicker::stopRecording()
{
    if (!m_recording) return;

    m_recording = false;

    // Снимаем перехват
    removeKeyboardHook();

    LOG_INFO(tr("Запись клавиатурного макроса завершена (действий: %1)")
                 .arg(m_recordedActions.size()));
    emit recordingStopped();
}

bool KeyboardClicker::isRecording() const
{
    return m_recording;
}

QList<KeyboardMacroAction> KeyboardClicker::recordedActions() const
{
    return m_recordedActions;
}

void KeyboardClicker::setRecordedActions(const QList<KeyboardMacroAction>& actions)
{
    m_recordedActions = actions;
}

void KeyboardClicker::clearRecordedActions()
{
    m_recordedActions.clear();
}

// ===========================================
// Конфигурация
// ===========================================

void KeyboardClicker::setConfig(const KeyboardClickerConfig& config)
{
    m_config = config;

    if (m_running && (m_config.mode == KeyboardMode::SingleKey ||
                       m_config.mode == KeyboardMode::KeyCombination)) {
        m_timer->setInterval(nextInterval());
    }
}

KeyboardClickerConfig KeyboardClicker::config() const
{
    return m_config;
}

// ===========================================
// Статистика
// ===========================================

qint64 KeyboardClicker::totalPresses() const
{
    return m_pressCount;
}

qint64 KeyboardClicker::elapsedMs() const
{
    if (m_running) {
        return m_elapsed.elapsed();
    }
    return 0;
}

// ===========================================
// Основной тик — SingleKey / KeyCombination
// ===========================================

void KeyboardClicker::onTimerTick()
{
    if (!m_running) return;

    if (!checkLimits()) {
        stop();
        emit finished();
        return;
    }

    performKeyPress();

    // Обновляем интервал при рандомизации
    if (m_config.randomizeInterval) {
        m_timer->setInterval(nextInterval());
    }

    // Обновляем статистику каждые 10 нажатий
    if (m_pressCount % 10 == 0) {
        emit statsUpdated(m_pressCount, m_elapsed.elapsed());
    }
}

void KeyboardClicker::performKeyPress()
{
    int vk = m_config.virtualKeyCode;

    if (m_config.mode == KeyboardMode::SingleKey) {
        // Простое нажатие одной клавиши с модификаторами
        QList<quint16> mods = buildModifiers();
        if (mods.isEmpty()) {
            m_input->keyPress(static_cast<WORD>(vk));
        } else {
            m_input->keyCombo(mods, static_cast<WORD>(vk));
        }
    } else if (m_config.mode == KeyboardMode::KeyCombination) {
        // Комбинация клавиш
        QList<quint16> mods = buildModifiers();
        m_input->keyCombo(mods, static_cast<WORD>(vk));
    }

    m_pressCount++;
    emit keyPressed(static_cast<int>(m_pressCount), vk);
}

// ===========================================
// Тик ввода текста — TypeText
// ===========================================

void KeyboardClicker::onTypeNextChar()
{
    if (!m_running) return;

    if (m_textIndex >= m_config.textToType.length()) {
        // Текст полностью введён

        // Если есть repeatCount — проверяем, повторяем ли
        if (m_config.repeatCount > 0) {
            m_pressCount++;
            if (m_pressCount >= m_config.repeatCount) {
                stop();
                emit finished();
                return;
            }
            // Начинаем ввод сначала
            m_textIndex = 0;
        } else if (m_config.repeatCount == 0) {
            // Бесконечный режим — начинаем сначала
            m_textIndex = 0;
            m_pressCount++;
        }

        emit statsUpdated(m_pressCount, m_elapsed.elapsed());
        return;
    }

    // Проверяем лимит по времени
    if (m_config.timeLimitMs > 0 && m_elapsed.elapsed() >= m_config.timeLimitMs) {
        stop();
        emit finished();
        return;
    }

    // Вводим следующий символ
    QChar ch = m_config.textToType.at(m_textIndex);
    m_input->typeChar(ch);
    m_textIndex++;

    // Рандомизация задержки между символами
    if (m_config.randomizeInterval) {
        int delay = QRandomGenerator::global()->bounded(
            m_config.randomIntervalMin, m_config.randomIntervalMax + 1);
        m_timer->setInterval(delay);
    }

    // Обновляем статистику
    if (m_textIndex % 10 == 0) {
        emit statsUpdated(m_pressCount, m_elapsed.elapsed());
    }
}

// ===========================================
// Тик воспроизведения макроса — MacroPlayback
// ===========================================

void KeyboardClicker::onPlayNextAction()
{
    if (!m_running) return;

    const auto& actions = m_config.macroActions;
    if (m_macroIndex >= actions.size()) {
        // Макрос закончился

        if (m_config.repeatCount > 0) {
            m_pressCount++;
            if (m_pressCount >= m_config.repeatCount) {
                stop();
                emit finished();
                return;
            }
            m_macroIndex = 0;  // Начинаем сначала
        } else if (m_config.repeatCount == 0) {
            // Бесконечный режим
            m_macroIndex = 0;
            m_pressCount++;
        }

        emit statsUpdated(m_pressCount, m_elapsed.elapsed());

        // Запускаем следующий цикл
        if (m_macroIndex < actions.size()) {
            m_timer->start(0);
        }
        return;
    }

    // Проверяем лимит по времени
    if (m_config.timeLimitMs > 0 && m_elapsed.elapsed() >= m_config.timeLimitMs) {
        stop();
        emit finished();
        return;
    }

    const auto& action = actions[m_macroIndex];

    switch (action.type) {
        case KeyboardMacroAction::Type::KeyDown:
            m_input->keyDown(static_cast<WORD>(action.vkCode));
            break;

        case KeyboardMacroAction::Type::KeyUp:
            m_input->keyUp(static_cast<WORD>(action.vkCode));
            break;

        case KeyboardMacroAction::Type::Delay:
            // Задержка будет обработана через интервал таймера
            break;
    }

    m_macroIndex++;

    // Вычисляем задержку до следующего действия
    if (m_macroIndex < actions.size()) {
        int delay = 0;
        if (actions[m_macroIndex].type == KeyboardMacroAction::Type::Delay) {
            delay = actions[m_macroIndex].delayMs;
            m_macroIndex++;  // Пропускаем действие задержки
        } else {
            // Вычисляем задержку по таймстемпам
            delay = static_cast<int>(actions[m_macroIndex].timestamp - action.timestamp);
            if (delay < 0) delay = 0;
        }
        m_timer->start(delay);
    } else {
        // Последнее действие — запускаем проверку повтора
        m_timer->start(100);
    }

    // Статистика
    if (m_macroIndex % 10 == 0) {
        emit statsUpdated(m_pressCount, m_elapsed.elapsed());
    }
}

// ===========================================
// Вычисление интервала
// ===========================================

int KeyboardClicker::nextInterval() const
{
    if (m_config.randomizeInterval) {
        int minI = m_config.randomIntervalMin;
        int maxI = m_config.randomIntervalMax;
        if (minI >= maxI) return minI;
        return QRandomGenerator::global()->bounded(minI, maxI + 1);
    }
    return m_config.intervalMs;
}

// ===========================================
// Проверка лимитов
// ===========================================

bool KeyboardClicker::checkLimits()
{
    // Лимит по количеству нажатий
    if (m_config.repeatCount > 0 && m_pressCount >= m_config.repeatCount) {
        LOG_INFO(tr("Достигнут лимит нажатий: %1").arg(m_config.repeatCount));
        return false;
    }

    // Лимит по времени
    if (m_config.timeLimitMs > 0 && m_elapsed.elapsed() >= m_config.timeLimitMs) {
        LOG_INFO(tr("Достигнут лимит времени: %1 мс").arg(m_config.timeLimitMs));
        return false;
    }

    return true;
}

// ===========================================
// Построение списка модификаторов
// ===========================================

QList<quint16> KeyboardClicker::buildModifiers() const
{
    QList<quint16> mods;
#ifdef Q_OS_WIN
    if (m_config.withCtrl)  mods.append(VK_CONTROL);
    if (m_config.withShift) mods.append(VK_SHIFT);
    if (m_config.withAlt)   mods.append(VK_MENU);
    if (m_config.withWin)   mods.append(VK_LWIN);
#endif
    return mods;
}

// ===========================================
// Преобразование VK-кода в имя клавиши (для GUI)
// ===========================================

QString KeyboardClicker::vkCodeToName(int vkCode)
{
#ifdef Q_OS_WIN
    // Получаем scan-код из VK-кода
    UINT scanCode = MapVirtualKey(static_cast<UINT>(vkCode), MAPVK_VK_TO_VSC);

    // Для расширенных клавиш (стрелки, Home, End, и т.д.)
    switch (vkCode) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= KF_EXTENDED;
            break;
    }

    wchar_t keyName[128] = {};
    int len = GetKeyNameTextW(static_cast<LONG>(scanCode << 16), keyName, 128);
    if (len > 0) {
        return QString::fromWCharArray(keyName, len);
    }

    // Фоллбэк для неизвестных клавиш
    switch (vkCode) {
        case VK_SPACE:   return "Space";
        case VK_RETURN:  return "Enter";
        case VK_TAB:     return "Tab";
        case VK_ESCAPE:  return "Escape";
        case VK_BACK:    return "Backspace";
        default:         return QString("VK_0x%1").arg(vkCode, 2, 16, QChar('0')).toUpper();
    }
#else
    return QString("VK_%1").arg(vkCode);
#endif
}

// ===========================================
// Win32 Low-Level Keyboard Hook для записи макросов
// ===========================================

void KeyboardClicker::installKeyboardHook()
{
#ifdef Q_OS_WIN
    if (s_keyboardHook) return;  // Уже установлен

    s_instance = this;
    s_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        keyboardHookProc,
        GetModuleHandle(nullptr),
        0  // Глобальный хук (все потоки)
    );

    if (!s_keyboardHook) {
        LOG_ERROR(tr("Не удалось установить keyboard hook (ошибка: %1)")
                      .arg(GetLastError()));
    } else {
        LOG_DEBUG(tr("Глобальный keyboard hook установлен"));
    }
#endif
}

void KeyboardClicker::removeKeyboardHook()
{
#ifdef Q_OS_WIN
    if (s_keyboardHook) {
        UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = nullptr;
        s_instance = nullptr;
        LOG_DEBUG(tr("Глобальный keyboard hook снят"));
    }
#endif
}

LRESULT CALLBACK KeyboardClicker::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifdef Q_OS_WIN
    if (nCode == HC_ACTION && s_instance && s_instance->m_recording) {
        KBDLLHOOKSTRUCT* kbs = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        // Игнорируем инъецированные события (чтобы не записывать собственные нажатия)
        if (kbs->flags & LLKHF_INJECTED) {
            return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
        }

        KeyboardMacroAction action;
        action.vkCode = static_cast<int>(kbs->vkCode);
        action.timestamp = s_instance->m_recordTimer.elapsed();

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            action.type = KeyboardMacroAction::Type::KeyDown;
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            action.type = KeyboardMacroAction::Type::KeyUp;
        } else {
            return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
        }

        // Добавляем задержку перед действием (если это не первое)
        if (!s_instance->m_recordedActions.isEmpty()) {
            qint64 prevTs = s_instance->m_recordedActions.last().timestamp;
            int delay = static_cast<int>(action.timestamp - prevTs);
            if (delay > 0) {
                KeyboardMacroAction delayAction;
                delayAction.type = KeyboardMacroAction::Type::Delay;
                delayAction.delayMs = delay;
                delayAction.timestamp = prevTs;
                s_instance->m_recordedActions.append(delayAction);
                emit s_instance->actionRecorded(delayAction);
            }
        }

        s_instance->m_recordedActions.append(action);
        emit s_instance->actionRecorded(action);
    }

    return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
#else
    Q_UNUSED(nCode); Q_UNUSED(wParam); Q_UNUSED(lParam);
    return 0;
#endif
}
