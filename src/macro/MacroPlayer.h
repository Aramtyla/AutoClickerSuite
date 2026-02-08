#pragma once
// ===========================================
// MacroPlayer.h — Воспроизведение макросов
// Поддерживает циклы, вложенные макросы,
// множитель скорости, паузу/стоп
// ===========================================

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QStack>
#include <QMap>

#include "MacroConfig.h"

class InputSimulator;

class MacroPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MacroPlayer(QObject* parent = nullptr);
    ~MacroPlayer();

    // ==========================================
    // Управление воспроизведением
    // ==========================================
    void play(const Macro& macro);          // Запустить макрос
    void stop();                             // Остановить
    void pause();                            // Приостановить
    void resume();                           // Возобновить
    void togglePlayStop();                   // Переключить воспроизведение/стоп
    bool isPlaying() const;
    bool isPaused() const;

    // ==========================================
    // Настройки
    // ==========================================
    void setSpeedMultiplier(double multiplier);  // Множитель скорости (0.1x — 10x)
    double speedMultiplier() const;

    // Библиотека макросов для вложенных вызовов (SubMacro)
    void setMacroLibrary(const QMap<QString, Macro>& library);

    // ==========================================
    // Статистика
    // ==========================================
    int  currentActionIndex() const;
    int  totalActions() const;
    int  currentRepeat() const;
    int  totalRepeats() const;
    qint64 elapsedMs() const;

signals:
    void playbackStarted();
    void playbackStopped();
    void playbackPaused();
    void playbackResumed();
    void playbackFinished();                   // Все повторы завершены
    void actionExecuted(int index, const MacroAction& action);
    void progressChanged(int current, int total);
    void repeatChanged(int current, int total);
    void error(const QString& message);

private slots:
    void executeNextAction();                  // Выполнить следующее действие

private:
    // Выполнить одно действие
    void executeAction(const MacroAction& action);

    // Вычислить задержку с учётом множителя скорости
    int adjustedDelay(int delayMs) const;

    // Поиск парного LoopEnd для LoopStart
    int findMatchingLoopEnd(int loopStartIndex) const;

    // Поиск парного LoopStart для LoopEnd (обратно)
    int findMatchingLoopStart(int loopEndIndex) const;

    // ==========================================
    // Структура состояния цикла
    // ==========================================
    struct LoopState {
        int startIndex;      // Индекс LoopStart
        int endIndex;        // Индекс LoopEnd
        int targetCount;     // Цель повторений (0 = бесконечно)
        int currentIter;     // Текущая итерация
    };

    // ==========================================
    // Состояние
    // ==========================================
    InputSimulator*         m_input          = nullptr;
    QTimer*                 m_timer          = nullptr;
    QElapsedTimer           m_elapsed;

    Macro                   m_macro;                    // Текущий макрос
    int                     m_actionIndex    = 0;       // Индекс текущего действия
    int                     m_currentRepeat  = 0;       // Текущее повторение

    bool                    m_playing        = false;
    bool                    m_paused         = false;
    double                  m_speed          = 1.0;     // Множитель скорости

    QStack<LoopState>       m_loopStack;                // Стек вложенных циклов
    int                     m_nestingDepth   = 0;       // Глубина вложенности

    // Библиотека макросов для SubMacro
    QMap<QString, Macro>    m_macroLibrary;
};
