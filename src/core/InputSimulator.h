#pragma once
// ===========================================
// InputSimulator.h — Обёртка над Win32 SendInput
// Единый интерфейс для симуляции ввода мыши и клавиатуры
// ===========================================

#include <QObject>
#include <QPoint>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class InputSimulator : public QObject
{
    Q_OBJECT

public:
    explicit InputSimulator(QObject* parent = nullptr);

    // ==========================================
    // Мышь
    // ==========================================

    // Клик левой/правой/средней кнопкой в текущей позиции
    void mouseClick(int button = 0);       // 0=Left, 1=Right, 2=Middle

    // Двойной клик
    void mouseDoubleClick(int button = 0);

    // Нажать кнопку (без отпускания)
    void mouseDown(int button = 0);

    // Отпустить кнопку
    void mouseUp(int button = 0);

    // Переместить курсор в абсолютные координаты
    void mouseMoveTo(int x, int y);

    // Переместить курсор относительно текущей позиции
    void mouseMoveRelative(int dx, int dy);

    // Получить текущую позицию курсора
    QPoint cursorPosition() const;

    // ==========================================
    // Клавиатура
    // ==========================================

    // Нажать и отпустить клавишу
    void keyPress(WORD vkCode);

    // Нажать клавишу (без отпускания)
    void keyDown(WORD vkCode);

    // Отпустить клавишу
    void keyUp(WORD vkCode);

    // Нажать комбинацию (например Ctrl+V)
    // modifiers — список VK-кодов модификаторов
    void keyCombo(const QList<WORD>& modifiers, WORD vkCode);

    // Ввести символ Unicode
    void typeChar(QChar ch);

    // Ввести строку (посимвольно)
    void typeString(const QString& text, int delayMs = 0);

private:
    // Отправка массива INPUT через SendInput
    void sendInputs(INPUT* inputs, UINT count);
};
