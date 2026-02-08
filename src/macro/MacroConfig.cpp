// ===========================================
// MacroConfig.cpp — Реализация описаний действий
// ===========================================

#include "MacroConfig.h"
#include "keyboard/KeyboardClicker.h"

#include <QObject>

QString MacroAction::description() const
{
    auto mouseButtonName = [](MacroMouseButton btn) -> QString {
        switch (btn) {
            case MacroMouseButton::Left:   return QObject::tr("ЛКМ");
            case MacroMouseButton::Right:  return QObject::tr("ПКМ");
            case MacroMouseButton::Middle: return QObject::tr("СКМ");
            default: return "?";
        }
    };

    switch (type) {
        case MacroActionType::MouseClick:
            if (useCurrentPos) {
                return QObject::tr("Клик %1 (текущая позиция)")
                    .arg(mouseButtonName(mouseButton));
            }
            return QObject::tr("Клик %1 (%2, %3)")
                .arg(mouseButtonName(mouseButton))
                .arg(position.x()).arg(position.y());

        case MacroActionType::MouseDoubleClick:
            if (useCurrentPos) {
                return QObject::tr("Двойной клик %1 (текущая позиция)")
                    .arg(mouseButtonName(mouseButton));
            }
            return QObject::tr("Двойной клик %1 (%2, %3)")
                .arg(mouseButtonName(mouseButton))
                .arg(position.x()).arg(position.y());

        case MacroActionType::MouseDown:
            return QObject::tr("Нажать %1").arg(mouseButtonName(mouseButton));

        case MacroActionType::MouseUp:
            return QObject::tr("Отпустить %1").arg(mouseButtonName(mouseButton));

        case MacroActionType::MouseMove:
            return QObject::tr("Переместить -> (%1, %2)")
                .arg(position.x()).arg(position.y());

        case MacroActionType::KeyDown:
            return QObject::tr("Нажать клавишу [%1]")
                .arg(KeyboardClicker::vkCodeToName(vkCode));

        case MacroActionType::KeyUp:
            return QObject::tr("Отпустить клавишу [%1]")
                .arg(KeyboardClicker::vkCodeToName(vkCode));

        case MacroActionType::KeyPress:
            return QObject::tr("Клавиша [%1]")
                .arg(KeyboardClicker::vkCodeToName(vkCode));

        case MacroActionType::KeyCombo: {
            QStringList parts;
            if (withCtrl)  parts << "Ctrl";
            if (withShift) parts << "Shift";
            if (withAlt)   parts << "Alt";
            if (withWin)   parts << "Win";
            parts << KeyboardClicker::vkCodeToName(vkCode);
            return QObject::tr("Комбинация [%1]").arg(parts.join("+"));
        }

        case MacroActionType::TypeText: {
            QString preview = text.left(20);
            if (text.length() > 20) preview += "...";
            return QObject::tr("Ввести текст: \"%1\"").arg(preview);
        }

        case MacroActionType::Delay:
            return QObject::tr("Задержка %1 мс").arg(delayMs);

        case MacroActionType::RandomDelay:
            return QObject::tr("Задержка %1-%2 мс").arg(delayMinMs).arg(delayMaxMs);

        case MacroActionType::LoopStart:
            if (loopCount == 0) {
                return QObject::tr("Начало цикла (бескон.)");
            }
            return QObject::tr("Начало цикла (x%1)").arg(loopCount);

        case MacroActionType::LoopEnd:
            return QObject::tr("Конец цикла");

        case MacroActionType::SubMacro:
            return QObject::tr("Макрос: %1")
                .arg(subMacroName.isEmpty() ? QObject::tr("(не задан)") : subMacroName);

        case MacroActionType::Comment:
            return QObject::tr("%1")
                .arg(comment.isEmpty() ? QObject::tr("(пусто)") : comment);

        default:
            return QObject::tr("? Неизвестное действие");
    }
}
