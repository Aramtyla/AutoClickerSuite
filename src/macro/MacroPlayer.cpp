// ===========================================
// MacroPlayer.cpp — Воспроизведение макросов
// с поддержкой циклов, вложенных макросов,
// множителя скорости, паузы/стопа
// ===========================================

#include "MacroPlayer.h"
#include "core/InputSimulator.h"
#include "utils/Logger.h"
#include "utils/Constants.h"

#include <QRandomGenerator>

MacroPlayer::MacroPlayer(QObject* parent)
    : QObject(parent)
{
    m_input = new InputSimulator(this);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);  // Каждое действие запускается по одиночному тику
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MacroPlayer::executeNextAction);
}

MacroPlayer::~MacroPlayer()
{
    stop();
}

// ===========================================
// Управление воспроизведением
// ===========================================

void MacroPlayer::play(const Macro& macro)
{
    if (m_playing) {
        stop();
    }

    m_macro         = macro;
    m_actionIndex   = 0;
    m_currentRepeat = 0;
    m_playing       = true;
    m_paused        = false;
    m_nestingDepth  = 0;
    m_loopStack.clear();
    m_elapsed.start();

    // Применяем множитель скорости из макроса (если задан)
    if (macro.speedMultiplier > 0.0) {
        m_speed = macro.speedMultiplier;
    }

    if (m_macro.actions.isEmpty()) {
        LOG_WARNING(tr("Макрос '%1' пуст — нечего воспроизводить").arg(m_macro.name));
        emit error(tr("Макрос пуст"));
        m_playing = false;
        return;
    }

    LOG_INFO(tr("Воспроизведение макроса '%1' (действий: %2, повторов: %3, скорость: x%4)")
                 .arg(m_macro.name)
                 .arg(m_macro.actions.size())
                 .arg(m_macro.repeatCount == 0 ? tr("∞") : QString::number(m_macro.repeatCount))
                 .arg(m_speed, 0, 'f', 1));

    emit playbackStarted();
    emit repeatChanged(m_currentRepeat + 1, m_macro.repeatCount);

    // Запускаем первое действие немедленно
    m_timer->start(0);
}

void MacroPlayer::stop()
{
    if (!m_playing) return;

    m_timer->stop();
    m_playing = false;
    m_paused  = false;
    m_loopStack.clear();

    qint64 elapsed = m_elapsed.elapsed();
    LOG_INFO(tr("Воспроизведение макроса '%1' остановлено (время: %2 мс)")
                 .arg(m_macro.name).arg(elapsed));

    emit playbackStopped();
}

void MacroPlayer::pause()
{
    if (!m_playing || m_paused) return;

    m_timer->stop();
    m_paused = true;

    LOG_INFO(tr("Воспроизведение макроса приостановлено"));
    emit playbackPaused();
}

void MacroPlayer::resume()
{
    if (!m_playing || !m_paused) return;

    m_paused = false;
    LOG_INFO(tr("Воспроизведение макроса возобновлено"));
    emit playbackResumed();

    // Продолжаем с текущего действия
    m_timer->start(0);
}

void MacroPlayer::togglePlayStop()
{
    if (m_playing) {
        stop();
    }
    // play() вызывается отдельно с макросом
}

bool MacroPlayer::isPlaying() const { return m_playing; }
bool MacroPlayer::isPaused() const { return m_paused; }

// ===========================================
// Настройки
// ===========================================

void MacroPlayer::setSpeedMultiplier(double multiplier)
{
    m_speed = qBound(0.1, multiplier, 10.0);
}

double MacroPlayer::speedMultiplier() const
{
    return m_speed;
}

void MacroPlayer::setMacroLibrary(const QMap<QString, Macro>& library)
{
    m_macroLibrary = library;
}

// ===========================================
// Статистика
// ===========================================

int MacroPlayer::currentActionIndex() const { return m_actionIndex; }
int MacroPlayer::totalActions() const { return m_macro.actions.size(); }
int MacroPlayer::currentRepeat() const { return m_currentRepeat; }
int MacroPlayer::totalRepeats() const { return m_macro.repeatCount; }

qint64 MacroPlayer::elapsedMs() const
{
    if (m_playing) return m_elapsed.elapsed();
    return 0;
}

// ===========================================
// Выполнение следующего действия
// ===========================================

void MacroPlayer::executeNextAction()
{
    if (!m_playing || m_paused) return;

    // Проверяем, закончился ли макрос
    if (m_actionIndex >= m_macro.actions.size()) {
        m_currentRepeat++;

        // Проверяем лимит повторений
        if (m_macro.repeatCount > 0 && m_currentRepeat >= m_macro.repeatCount) {
            LOG_INFO(tr("Макрос '%1' полностью завершён (%2 повторов)")
                         .arg(m_macro.name).arg(m_currentRepeat));
            m_playing = false;
            emit playbackFinished();
            emit playbackStopped();
            return;
        }

        // Начинаем новый повтор
        m_actionIndex = 0;
        m_loopStack.clear();
        emit repeatChanged(m_currentRepeat + 1, m_macro.repeatCount);
    }

    // Получаем текущее действие
    const MacroAction& action = m_macro.actions[m_actionIndex];

    // Обработка управляющих конструкций (не вызывают executeAction)
    int delay = 0;

    switch (action.type) {
        case MacroActionType::LoopStart: {
            // Ищем парный LoopEnd
            int endIdx = findMatchingLoopEnd(m_actionIndex);
            if (endIdx < 0) {
                LOG_ERROR(tr("Не найден LoopEnd для LoopStart на позиции %1").arg(m_actionIndex));
                emit error(tr("Ошибка структуры цикла: нет LoopEnd"));
                stop();
                return;
            }

            // Проверяем глубину вложенности
            if (m_nestingDepth >= AppConstants::Macro::MAX_NESTED_DEPTH) {
                LOG_ERROR(tr("Превышена максимальная глубина вложенности: %1")
                              .arg(AppConstants::Macro::MAX_NESTED_DEPTH));
                emit error(tr("Слишком глубокая вложенность циклов"));
                stop();
                return;
            }

            // Помещаем состояние цикла в стек
            LoopState loop;
            loop.startIndex  = m_actionIndex;
            loop.endIndex    = endIdx;
            loop.targetCount = action.loopCount;
            loop.currentIter = 0;
            m_loopStack.push(loop);
            m_nestingDepth++;

            m_actionIndex++;
            delay = 0;
            break;
        }

        case MacroActionType::LoopEnd: {
            if (m_loopStack.isEmpty()) {
                LOG_ERROR(tr("LoopEnd без LoopStart на позиции %1").arg(m_actionIndex));
                emit error(tr("Ошибка структуры цикла: LoopEnd без LoopStart"));
                stop();
                return;
            }

            LoopState& loop = m_loopStack.top();
            loop.currentIter++;

            // Проверяем, завершился ли цикл
            bool loopDone = false;
            if (loop.targetCount > 0 && loop.currentIter >= loop.targetCount) {
                loopDone = true;
            }
            // targetCount == 0 → бесконечный цикл (завершится только по стопу)

            if (loopDone) {
                m_loopStack.pop();
                m_nestingDepth--;
                m_actionIndex++;  // Переходим к следующему за LoopEnd
            } else {
                // Возвращаемся к началу цикла (к действию после LoopStart)
                m_actionIndex = loop.startIndex + 1;
            }

            delay = 0;
            break;
        }

        case MacroActionType::Delay: {
            delay = adjustedDelay(action.delayMs);
            m_actionIndex++;
            emit actionExecuted(m_actionIndex - 1, action);
            break;
        }

        case MacroActionType::RandomDelay: {
            int rndDelay = QRandomGenerator::global()->bounded(
                action.delayMinMs, action.delayMaxMs + 1);
            delay = adjustedDelay(rndDelay);
            m_actionIndex++;
            emit actionExecuted(m_actionIndex - 1, action);
            break;
        }

        case MacroActionType::SubMacro: {
            // Ищем вложенный макрос в библиотеке
            if (m_macroLibrary.contains(action.subMacroName)) {
                const Macro& subMacro = m_macroLibrary[action.subMacroName];

                // Проверяем глубину вложенности
                if (m_nestingDepth >= AppConstants::Macro::MAX_NESTED_DEPTH) {
                    LOG_WARNING(tr("Пропуск вложенного макроса '%1': превышена глубина")
                                    .arg(action.subMacroName));
                } else {
                    // Вставляем действия вложенного макроса в текущий поток
                    // (простой линейный подход — разворачиваем вложенный макрос)
                    LOG_INFO(tr("Выполнение вложенного макроса '%1' (%2 действий)")
                                 .arg(action.subMacroName).arg(subMacro.actions.size()));

                    // Создаём временный список с вставкой
                    QList<MacroAction> expanded;
                    for (int i = 0; i < m_actionIndex + 1; ++i) {
                        expanded.append(m_macro.actions[i]);
                    }
                    for (const auto& subAction : subMacro.actions) {
                        expanded.append(subAction);
                    }
                    for (int i = m_actionIndex + 1; i < m_macro.actions.size(); ++i) {
                        expanded.append(m_macro.actions[i]);
                    }
                    m_macro.actions = expanded;
                    m_nestingDepth++;
                }
            } else {
                LOG_WARNING(tr("Вложенный макрос '%1' не найден в библиотеке")
                                .arg(action.subMacroName));
            }
            m_actionIndex++;
            delay = 0;
            break;
        }

        case MacroActionType::Comment: {
            // Комментарии пропускаются
            m_actionIndex++;
            delay = 0;
            break;
        }

        default: {
            // Все остальные действия — выполняем
            executeAction(action);
            emit actionExecuted(m_actionIndex, action);
            m_actionIndex++;
            delay = 1;  // Минимальная задержка между действиями
            break;
        }
    }

    // Обновляем прогресс
    emit progressChanged(m_actionIndex, m_macro.actions.size());

    // Планируем следующее действие
    if (m_playing && !m_paused) {
        m_timer->start(qMax(0, delay));
    }
}

// ===========================================
// Выполнение одного действия
// ===========================================

void MacroPlayer::executeAction(const MacroAction& action)
{
    switch (action.type) {
        case MacroActionType::MouseClick: {
            if (!action.useCurrentPos) {
                m_input->mouseMoveTo(action.position.x(), action.position.y());
            }
            m_input->mouseClick(static_cast<int>(action.mouseButton));
            break;
        }

        case MacroActionType::MouseDoubleClick: {
            if (!action.useCurrentPos) {
                m_input->mouseMoveTo(action.position.x(), action.position.y());
            }
            m_input->mouseDoubleClick(static_cast<int>(action.mouseButton));
            break;
        }

        case MacroActionType::MouseDown: {
            if (!action.useCurrentPos) {
                m_input->mouseMoveTo(action.position.x(), action.position.y());
            }
            m_input->mouseDown(static_cast<int>(action.mouseButton));
            break;
        }

        case MacroActionType::MouseUp: {
            if (!action.useCurrentPos) {
                m_input->mouseMoveTo(action.position.x(), action.position.y());
            }
            m_input->mouseUp(static_cast<int>(action.mouseButton));
            break;
        }

        case MacroActionType::MouseMove: {
            m_input->mouseMoveTo(action.position.x(), action.position.y());
            break;
        }

        case MacroActionType::KeyDown: {
            m_input->keyDown(static_cast<WORD>(action.vkCode));
            break;
        }

        case MacroActionType::KeyUp: {
            m_input->keyUp(static_cast<WORD>(action.vkCode));
            break;
        }

        case MacroActionType::KeyPress: {
            m_input->keyPress(static_cast<WORD>(action.vkCode));
            break;
        }

        case MacroActionType::KeyCombo: {
            QList<WORD> mods;
            if (action.withCtrl)  mods.append(VK_CONTROL);
            if (action.withShift) mods.append(VK_SHIFT);
            if (action.withAlt)   mods.append(VK_MENU);
            if (action.withWin)   mods.append(VK_LWIN);
            m_input->keyCombo(mods, static_cast<WORD>(action.vkCode));
            break;
        }

        case MacroActionType::TypeText: {
            m_input->typeString(action.text, adjustedDelay(action.delayMs));
            break;
        }

        default:
            break;
    }
}

// ===========================================
// Задержка с учётом множителя скорости
// ===========================================

int MacroPlayer::adjustedDelay(int delayMs) const
{
    if (m_speed <= 0.0) return delayMs;
    return qMax(0, static_cast<int>(delayMs / m_speed));
}

// ===========================================
// Поиск парных LoopStart / LoopEnd
// ===========================================

int MacroPlayer::findMatchingLoopEnd(int loopStartIndex) const
{
    int depth = 0;
    for (int i = loopStartIndex; i < m_macro.actions.size(); ++i) {
        if (m_macro.actions[i].type == MacroActionType::LoopStart) {
            depth++;
        } else if (m_macro.actions[i].type == MacroActionType::LoopEnd) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    return -1;  // Не найден
}

int MacroPlayer::findMatchingLoopStart(int loopEndIndex) const
{
    int depth = 0;
    for (int i = loopEndIndex; i >= 0; --i) {
        if (m_macro.actions[i].type == MacroActionType::LoopEnd) {
            depth++;
        } else if (m_macro.actions[i].type == MacroActionType::LoopStart) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    return -1;  // Не найден
}
