// ===========================================
// InputSimulator.cpp — Реализация симуляции ввода
// через Win32 SendInput API
// ===========================================

#include "InputSimulator.h"
#include "utils/Logger.h"

#include <QThread>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

InputSimulator::InputSimulator(QObject* parent)
    : QObject(parent)
{
}

// ===========================================
// Мышь
// ===========================================

void InputSimulator::mouseClick(int button)
{
    mouseDown(button);
    mouseUp(button);
}

void InputSimulator::mouseDoubleClick(int button)
{
    mouseClick(button);
    mouseClick(button);
}

void InputSimulator::mouseDown(int button)
{
#ifdef Q_OS_WIN
    INPUT input = {};
    input.type = INPUT_MOUSE;

    switch (button) {
        case 0: input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;   break;
        case 1: input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
        case 2: input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
        default: return;
    }

    sendInputs(&input, 1);
#endif
}

void InputSimulator::mouseUp(int button)
{
#ifdef Q_OS_WIN
    INPUT input = {};
    input.type = INPUT_MOUSE;

    switch (button) {
        case 0: input.mi.dwFlags = MOUSEEVENTF_LEFTUP;   break;
        case 1: input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;  break;
        case 2: input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
        default: return;
    }

    sendInputs(&input, 1);
#endif
}

void InputSimulator::mouseMoveTo(int x, int y)
{
#ifdef Q_OS_WIN
    // Нормализация координат для MOUSEEVENTF_ABSOLUTE
    // (0..65535 соответствует полному разрешению экрана)
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = static_cast<LONG>((x * 65535) / screenW);
    input.mi.dy = static_cast<LONG>((y * 65535) / screenH);

    sendInputs(&input, 1);
#endif
}

void InputSimulator::mouseMoveRelative(int dx, int dy)
{
#ifdef Q_OS_WIN
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = dx;
    input.mi.dy = dy;

    sendInputs(&input, 1);
#endif
}

QPoint InputSimulator::cursorPosition() const
{
#ifdef Q_OS_WIN
    POINT pt;
    if (GetCursorPos(&pt)) {
        return QPoint(pt.x, pt.y);
    }
#endif
    return QPoint(0, 0);
}

// ===========================================
// Клавиатура
// ===========================================

void InputSimulator::keyPress(WORD vkCode)
{
    keyDown(vkCode);
    keyUp(vkCode);
}

void InputSimulator::keyDown(WORD vkCode)
{
#ifdef Q_OS_WIN
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.wScan = static_cast<WORD>(MapVirtualKey(vkCode, MAPVK_VK_TO_VSC));

    sendInputs(&input, 1);
#endif
}

void InputSimulator::keyUp(WORD vkCode)
{
#ifdef Q_OS_WIN
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.wScan = static_cast<WORD>(MapVirtualKey(vkCode, MAPVK_VK_TO_VSC));
    input.ki.dwFlags = KEYEVENTF_KEYUP;

    sendInputs(&input, 1);
#endif
}

void InputSimulator::keyCombo(const QList<WORD>& modifiers, WORD vkCode)
{
    // Нажимаем модификаторы
    for (WORD mod : modifiers) {
        keyDown(mod);
    }

    // Нажимаем основную клавишу
    keyPress(vkCode);

    // Отпускаем модификаторы в обратном порядке
    for (int i = modifiers.size() - 1; i >= 0; --i) {
        keyUp(modifiers[i]);
    }
}

void InputSimulator::typeChar(QChar ch)
{
#ifdef Q_OS_WIN
    INPUT inputs[2] = {};

    // Нажатие
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = 0;
    inputs[0].ki.wScan = ch.unicode();
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

    // Отпускание
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0;
    inputs[1].ki.wScan = ch.unicode();
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    sendInputs(inputs, 2);
#endif
}

void InputSimulator::typeString(const QString& text, int delayMs)
{
    for (const QChar& ch : text) {
        typeChar(ch);
        if (delayMs > 0) {
            QThread::msleep(static_cast<unsigned long>(delayMs));
        }
    }
}

// ===========================================
// Вспомогательные
// ===========================================

void InputSimulator::sendInputs(INPUT* inputs, UINT count)
{
#ifdef Q_OS_WIN
    UINT sent = SendInput(count, inputs, sizeof(INPUT));
    if (sent != count) {
        LOG_WARNING(tr("SendInput: отправлено %1 из %2 событий (ошибка: %3)")
                        .arg(sent).arg(count).arg(GetLastError()));
    }
#endif
}
