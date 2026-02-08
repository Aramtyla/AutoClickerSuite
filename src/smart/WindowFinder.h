#pragma once
// ===========================================
// WindowFinder.h — Поиск и привязка к окнам Windows
// Перечисление окон, захват дескриптора, проверка
// активности, получение координат
// ===========================================

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// Информация об окне
struct WindowInfo
{
    quintptr handle  = 0;        // HWND
    QString  title;              // Заголовок окна
    QString  className;          // Класс окна
    QRect    geometry;           // Позиция и размер
    bool     isVisible = false;  // Видимо ли
    DWORD    processId = 0;      // PID процесса
};

class WindowFinder : public QObject
{
    Q_OBJECT

public:
    explicit WindowFinder(QObject* parent = nullptr);

    // ==========================================
    // Перечисление окон
    // ==========================================

    // Получить список всех видимых окон верхнего уровня
    QList<WindowInfo> enumerateWindows() const;

    // Найти окна по заголовку (частичное совпадение)
    QList<WindowInfo> findByTitle(const QString& titleFragment) const;

    // Найти окна по имени класса
    QList<WindowInfo> findByClass(const QString& className) const;

    // ==========================================
    // Работа с конкретным окном
    // ==========================================

    // Установить целевое окно
    void setTargetWindow(quintptr hwnd);
    void setTargetByTitle(const QString& titleFragment);

    // Получить информацию о текущем целевом окне
    WindowInfo targetWindowInfo() const;

    // Проверить, существует ли окно и видно ли оно
    bool isTargetValid() const;
    bool isTargetForeground() const;

    // ==========================================
    // Координаты и фокус
    // ==========================================

    // Перевести глобальные координаты в координаты клиентской области окна
    QPoint screenToClient(const QPoint& screenPos) const;

    // Перевести координаты клиентской области в глобальные
    QPoint clientToScreen(const QPoint& clientPos) const;

    // Получить прямоугольник клиентской области
    QRect clientRect() const;

    // Вывести окно на передний план
    void bringToForeground();

    // ==========================================
    // Захват окна
    // ==========================================

    // Сделать скриншот целевого окна
    QPixmap captureWindow() const;

    // Сделать скриншот области экрана
    static QPixmap captureScreen(const QRect& area = QRect());

    // ==========================================
    // Режим выбора окна (pick window)
    // ==========================================

    // Начать режим выбора окна (пользователь кликает на окно)
    void startPickWindow();

    // Проверка — в режиме ли выбора
    bool isPickingWindow() const { return m_pickingWindow; }

    quintptr targetHandle() const { return m_targetHwnd; }

signals:
    // Окно выбрано пользователем (через pick)
    void windowPicked(quintptr hwnd, const QString& title);

    // Целевое окно потеряно (закрыто, стало невидимо)
    void targetLost();

    // Целевое окно найдено/стало доступно
    void targetFound();

private:
    quintptr m_targetHwnd    = 0;
    bool     m_pickingWindow = false;
    QTimer*  m_pickTimer     = nullptr;

    // Win32 callback для EnumWindows
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);

    // Получить информацию об окне по HWND
    static WindowInfo getWindowInfo(HWND hwnd);
};

