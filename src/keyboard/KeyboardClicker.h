#pragma once
// ===========================================
// KeyboardClicker.h — Движок автонажатия клавиатуры
// Поддерживает 4 режима: одиночная клавиша, комбинация,
// посимвольный ввод текста, воспроизведение макроса
// ===========================================

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include <QRandomGenerator>

#include "KeyboardConfig.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

class InputSimulator;

class KeyboardClicker : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardClicker(QObject* parent = nullptr);
    ~KeyboardClicker();

    // ==========================================
    // Управление
    // ==========================================
    void start();             // Запустить автонажатие
    void stop();              // Остановить автонажатие
    void toggleStartStop();   // Переключить старт/стоп
    bool isRunning() const;

    // ==========================================
    // Запись макроса
    // ==========================================
    void startRecording();    // Начать запись нажатий
    void stopRecording();     // Остановить запись
    bool isRecording() const;

    // Получить записанные действия
    QList<KeyboardMacroAction> recordedActions() const;
    void setRecordedActions(const QList<KeyboardMacroAction>& actions);
    void clearRecordedActions();

    // ==========================================
    // Конфигурация
    // ==========================================
    void setConfig(const KeyboardClickerConfig& config);
    KeyboardClickerConfig config() const;

    // Статистика
    qint64 totalPresses() const;
    qint64 elapsedMs() const;

    // Получить имя клавиши по VK-коду (для GUI)
    static QString vkCodeToName(int vkCode);

signals:
    void started();
    void stopped();
    void keyPressed(int pressNumber, int vkCode);
    void finished();           // Завершение по лимиту
    void error(const QString& message);
    void statsUpdated(qint64 presses, qint64 elapsedMs);

    // Сигналы записи
    void recordingStarted();
    void recordingStopped();
    void actionRecorded(const KeyboardMacroAction& action);

private slots:
    void onTimerTick();        // Основной тик (SingleKey, KeyCombination)
    void onTypeNextChar();     // Тик ввода текста
    void onPlayNextAction();   // Тик воспроизведения макроса

private:
    // Выполнить одиночное нажатие или комбинацию
    void performKeyPress();

    // Рассчитать интервал с учётом рандомизации
    int nextInterval() const;

    // Проверить лимиты (по количеству/времени)
    bool checkLimits();

    // Построить список VK-кодов модификаторов
    QList<quint16> buildModifiers() const;

    // Win32 low-level keyboard hook для записи макросов
    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    void installKeyboardHook();
    void removeKeyboardHook();

    // ==========================================
    // Состояние
    // ==========================================
    InputSimulator*         m_input         = nullptr;
    QTimer*                 m_timer         = nullptr;
    QElapsedTimer           m_elapsed;
    KeyboardClickerConfig   m_config;
    bool                    m_running       = false;

    // Статистика
    qint64                  m_pressCount    = 0;

    // Режим ввода текста
    int                     m_textIndex     = 0;

    // Режим воспроизведения макроса
    int                     m_macroIndex    = 0;

    // Запись макроса
    bool                    m_recording     = false;
    QElapsedTimer           m_recordTimer;
    QList<KeyboardMacroAction> m_recordedActions;

    // Win32 hook
    static HHOOK            s_keyboardHook;
    static KeyboardClicker* s_instance;      // Для доступа из статического callback
};
