#pragma once
// ===========================================
// MacroRecorder.h — Запись макросов в реальном времени
// Перехватывает события мыши и клавиатуры через
// Win32 low-level hooks, формируя список MacroAction
// ===========================================

#include <QObject>
#include <QElapsedTimer>
#include <QList>
#include <QPoint>

#include "MacroConfig.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class MacroRecorder : public QObject
{
    Q_OBJECT

public:
    explicit MacroRecorder(QObject* parent = nullptr);
    ~MacroRecorder();

    // ==========================================
    // Управление записью
    // ==========================================
    void startRecording();         // Начать запись
    void stopRecording();          // Остановить запись
    void pauseRecording();         // Приостановить запись
    void resumeRecording();        // Возобновить запись
    bool isRecording() const;
    bool isPaused() const;

    // ==========================================
    // Настройки записи
    // ==========================================
    void setRecordMouse(bool enabled);       // Записывать мышь
    void setRecordKeyboard(bool enabled);    // Записывать клавиатуру
    void setRecordMouseMove(bool enabled);   // Записывать перемещения мыши
    void setMinMoveDistance(int pixels);      // Мин. расстояние перемещения для записи

    bool recordMouse() const;
    bool recordKeyboard() const;
    bool recordMouseMove() const;

    // ==========================================
    // Результат
    // ==========================================
    QList<MacroAction> recordedActions() const;
    void clearRecordedActions();
    int actionCount() const;

signals:
    void recordingStarted();
    void recordingStopped();
    void recordingPaused();
    void recordingResumed();
    void actionRecorded(const MacroAction& action);    // Новое действие записано
    void error(const QString& message);

private:
    // Win32 hook callbacks
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    void installHooks();
    void removeHooks();

    // Добавить действие с автоматической задержкой
    void appendAction(const MacroAction& action);

    // Проверка минимального расстояния перемещения
    bool isSignificantMove(const QPoint& newPos) const;

    // ==========================================
    // Состояние
    // ==========================================
    bool                    m_recording       = false;
    bool                    m_paused          = false;
    QElapsedTimer           m_timer;                     // Таймер для вычисления задержек
    qint64                  m_pauseOffset     = 0;       // Смещение времени паузы
    qint64                  m_pauseStart      = 0;       // Начало текущей паузы
    QList<MacroAction>      m_actions;                   // Записанные действия

    // Настройки записи
    bool                    m_recordMouse     = true;
    bool                    m_recordKeyboard  = true;
    bool                    m_recordMouseMove = true;
    int                     m_minMoveDistance = 5;        // Пикселей

    // Последняя позиция мыши (для фильтрации перемещений)
    QPoint                  m_lastMousePos;

    // Win32 hooks
    static HHOOK            s_mouseHook;
    static HHOOK            s_keyboardHook;
    static MacroRecorder*   s_instance;
};
