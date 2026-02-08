// ===========================================
// WindowFinder.cpp — Реализация поиска и привязки к окнам
// Win32 API: EnumWindows, GetWindowText,
// GetClassName, SetForegroundWindow, PrintWindow
// ===========================================

#include "WindowFinder.h"
#include "utils/Logger.h"

#include <QScreen>
#include <QGuiApplication>
#include <QWindow>

WindowFinder::WindowFinder(QObject* parent)
    : QObject(parent)
{
    // Таймер для режима выбора окна (проверяем клик каждые 50мс)
    m_pickTimer = new QTimer(this);
    m_pickTimer->setInterval(50);
    connect(m_pickTimer, &QTimer::timeout, this, [this]() {
        // Проверяем, зажата ли ЛКМ
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            // Получаем окно под курсором
            POINT pt;
            GetCursorPos(&pt);
            HWND hwnd = WindowFromPoint(pt);

            // Ищем верхнеуровневое окно
            HWND topLevel = GetAncestor(hwnd, GA_ROOT);
            if (topLevel) {
                hwnd = topLevel;
            }

            // Игнорируем наше собственное окно
            HWND ourWindow = reinterpret_cast<HWND>(
                QGuiApplication::allWindows().isEmpty() ? 0 :
                QGuiApplication::allWindows().first()->winId());

            if (hwnd && hwnd != ourWindow) {
                setTargetWindow(reinterpret_cast<quintptr>(hwnd));
                m_pickingWindow = false;
                m_pickTimer->stop();

                WindowInfo info = getWindowInfo(hwnd);
                emit windowPicked(reinterpret_cast<quintptr>(hwnd), info.title);
                LOG_INFO(tr("Окно выбрано: %1 (HWND: 0x%2)")
                    .arg(info.title)
                    .arg(reinterpret_cast<quintptr>(hwnd), 0, 16));
            }
        }

        // Отмена по Escape
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            m_pickingWindow = false;
            m_pickTimer->stop();
            LOG_INFO(tr("Выбор окна отменён"));
        }
    });
}

// ==========================================
// Перечисление окон
// ==========================================

BOOL CALLBACK WindowFinder::enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* list = reinterpret_cast<QList<WindowInfo>*>(lParam);

    // Пропускаем невидимые окна
    if (!IsWindowVisible(hwnd)) return TRUE;

    // Пропускаем окна с пустым заголовком
    wchar_t title[512];
    int len = GetWindowTextW(hwnd, title, 512);
    if (len == 0) return TRUE;

    // Пропускаем окна без заголовка (tool windows, etc.)
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    list->append(getWindowInfo(hwnd));
    return TRUE;
}

WindowInfo WindowFinder::getWindowInfo(HWND hwnd)
{
    WindowInfo info;
    info.handle = reinterpret_cast<quintptr>(hwnd);

    // Заголовок
    wchar_t title[512];
    GetWindowTextW(hwnd, title, 512);
    info.title = QString::fromWCharArray(title);

    // Класс окна
    wchar_t className[256];
    GetClassNameW(hwnd, className, 256);
    info.className = QString::fromWCharArray(className);

    // Геометрия
    RECT rect;
    GetWindowRect(hwnd, &rect);
    info.geometry = QRect(rect.left, rect.top,
                          rect.right - rect.left,
                          rect.bottom - rect.top);

    info.isVisible = IsWindowVisible(hwnd);

    // PID
    GetWindowThreadProcessId(hwnd, &info.processId);

    return info;
}

QList<WindowInfo> WindowFinder::enumerateWindows() const
{
    QList<WindowInfo> windows;
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

QList<WindowInfo> WindowFinder::findByTitle(const QString& titleFragment) const
{
    QList<WindowInfo> result;
    QList<WindowInfo> all = enumerateWindows();
    for (const auto& win : all) {
        if (win.title.contains(titleFragment, Qt::CaseInsensitive)) {
            result.append(win);
        }
    }
    return result;
}

QList<WindowInfo> WindowFinder::findByClass(const QString& className) const
{
    QList<WindowInfo> result;
    QList<WindowInfo> all = enumerateWindows();
    for (const auto& win : all) {
        if (win.className.compare(className, Qt::CaseInsensitive) == 0) {
            result.append(win);
        }
    }
    return result;
}

// ==========================================
// Работа с целевым окном
// ==========================================

void WindowFinder::setTargetWindow(quintptr hwnd)
{
    m_targetHwnd = hwnd;
    if (isTargetValid()) {
        emit targetFound();
    }
}

void WindowFinder::setTargetByTitle(const QString& titleFragment)
{
    auto windows = findByTitle(titleFragment);
    if (!windows.isEmpty()) {
        setTargetWindow(windows.first().handle);
    }
}

WindowInfo WindowFinder::targetWindowInfo() const
{
    if (m_targetHwnd == 0) return {};
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    if (!IsWindow(hwnd)) return {};
    return getWindowInfo(hwnd);
}

bool WindowFinder::isTargetValid() const
{
    if (m_targetHwnd == 0) return false;
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    return IsWindow(hwnd) && IsWindowVisible(hwnd);
}

bool WindowFinder::isTargetForeground() const
{
    if (m_targetHwnd == 0) return false;
    return GetForegroundWindow() == reinterpret_cast<HWND>(m_targetHwnd);
}

// ==========================================
// Координаты и фокус
// ==========================================

QPoint WindowFinder::screenToClient(const QPoint& screenPos) const
{
    if (m_targetHwnd == 0) return screenPos;
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    POINT pt = {screenPos.x(), screenPos.y()};
    ScreenToClient(hwnd, &pt);
    return QPoint(pt.x, pt.y);
}

QPoint WindowFinder::clientToScreen(const QPoint& clientPos) const
{
    if (m_targetHwnd == 0) return clientPos;
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    POINT pt = {clientPos.x(), clientPos.y()};
    ClientToScreen(hwnd, &pt);
    return QPoint(pt.x, pt.y);
}

QRect WindowFinder::clientRect() const
{
    if (m_targetHwnd == 0) return {};
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Преобразуем в экранные координаты
    POINT topLeft = {rect.left, rect.top};
    ClientToScreen(hwnd, &topLeft);

    return QRect(topLeft.x, topLeft.y,
                 rect.right - rect.left,
                 rect.bottom - rect.top);
}

void WindowFinder::bringToForeground()
{
    if (m_targetHwnd == 0) return;
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);

    // Если окно свёрнуто — восстанавливаем
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }

    // Выводим на передний план
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
}

// ==========================================
// Захват экрана/окна
// ==========================================

QPixmap WindowFinder::captureWindow() const
{
    if (m_targetHwnd == 0) return {};
    HWND hwnd = reinterpret_cast<HWND>(m_targetHwnd);
    if (!IsWindow(hwnd)) return {};

    RECT rect;
    GetClientRect(hwnd, &rect);
    int w = rect.right  - rect.left;
    int h = rect.bottom - rect.top;
    if (w <= 0 || h <= 0) return {};

    // Создаём DC и битмап для захвата
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem    = CreateCompatibleDC(hdcWindow);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, w, h);
    SelectObject(hdcMem, hBitmap);

    // Пытаемся PrintWindow (работает даже для частично перекрытых окон)
    if (!PrintWindow(hwnd, hdcMem, PW_CLIENTONLY)) {
        // Fallback — BitBlt
        BitBlt(hdcMem, 0, 0, w, h, hdcWindow, 0, 0, SRCCOPY);
    }

    // Конвертируем HBITMAP → QPixmap
    // Получаем данные bitmap
    BITMAPINFOHEADER bi = {};
    bi.biSize        = sizeof(BITMAPINFOHEADER);
    bi.biWidth       = w;
    bi.biHeight      = -h;  // Top-down
    bi.biPlanes      = 1;
    bi.biBitCount    = 32;
    bi.biCompression = BI_RGB;

    QImage image(w, h, QImage::Format_ARGB32);
    GetDIBits(hdcMem, hBitmap, 0, h,
              image.bits(), reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

    // Очистка
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);

    return QPixmap::fromImage(image);
}

QPixmap WindowFinder::captureScreen(const QRect& area)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return {};

    if (area.isNull() || !area.isValid()) {
        return screen->grabWindow(0); // Весь экран
    }

    return screen->grabWindow(0, area.x(), area.y(),
                              area.width(), area.height());
}

// ==========================================
// Режим выбора окна
// ==========================================

void WindowFinder::startPickWindow()
{
    m_pickingWindow = true;
    m_pickTimer->start();
    LOG_INFO(tr("Кликните по целевому окну (Esc — отмена)..."));
}

