#pragma once
// ===========================================
// MacroEditor.h — Визуальный редактор макросов
// Drag & drop список действий, добавление/удаление/
// редактирование параметров каждого действия
// ===========================================

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMenu>

#include "MacroConfig.h"

class MacroEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MacroEditor(QWidget* parent = nullptr);

    // ==========================================
    // Работа с действиями
    // ==========================================
    void setActions(const QList<MacroAction>& actions);
    QList<MacroAction> actions() const;

    void addAction(const MacroAction& action);
    void insertAction(int index, const MacroAction& action);
    void removeAction(int index);
    void updateAction(int index, const MacroAction& action);
    void clearActions();
    int  actionCount() const;

    // Перемещение действий
    void moveActionUp(int index);
    void moveActionDown(int index);

    // Выделение
    int  selectedIndex() const;
    void selectAction(int index);

    // Подсветка текущего исполняемого действия
    void highlightAction(int index);
    void clearHighlight();

signals:
    void actionsChanged();                     // Список действий изменился
    void actionSelected(int index);            // Выбрано действие

private slots:
    void onItemSelectionChanged();             // Выбор в списке
    void onAddActionMenu();                    // Контекстное меню добавления
    void onRemoveSelected();                   // Удалить выбранное
    void onMoveUp();                           // Сдвинуть вверх
    void onMoveDown();                         // Сдвинуть вниз
    void onDuplicateSelected();                // Дублировать
    void onEditApplied();                      // Применить изменения параметров

private:
    void setupUI();
    void setupActionList();
    void setupParamsPanel();
    void setupToolbar();
    void createAddMenu();

    // Обновить элемент списка
    void updateListItem(int index);

    // Загрузить параметры действия в панель редактирования
    void loadActionParams(int index);

    // Получить иконку/цвет для типа действия
    QIcon actionIcon(MacroActionType type) const;
    QString actionColor(MacroActionType type) const;

    // Создание действия с параметрами по умолчанию
    MacroAction createDefaultAction(MacroActionType type) const;

    // Добавить действие определённого типа
    void addActionOfType(MacroActionType type);

    // ==========================================
    // Данные
    // ==========================================
    QList<MacroAction>    m_actions;

    // ==========================================
    // Виджеты — список действий
    // ==========================================
    QListWidget*          m_actionList    = nullptr;

    // ==========================================
    // Тулбар
    // ==========================================
    QPushButton*          m_btnAdd        = nullptr;
    QPushButton*          m_btnRemove     = nullptr;
    QPushButton*          m_btnMoveUp     = nullptr;
    QPushButton*          m_btnMoveDown   = nullptr;
    QPushButton*          m_btnDuplicate  = nullptr;
    QMenu*                m_addMenu       = nullptr;

    // ==========================================
    // Панель параметров
    // ==========================================
    QGroupBox*            m_paramsGroup   = nullptr;
    QStackedWidget*       m_paramsStack   = nullptr;

    // Страница: Мышь (клик / двойной / down / up)
    QComboBox*            m_mouseButtonCombo = nullptr;
    QCheckBox*            m_useCurrentPosCheck = nullptr;
    QSpinBox*             m_posXSpin      = nullptr;
    QSpinBox*             m_posYSpin      = nullptr;

    // Страница: Клавиатура
    QSpinBox*             m_vkCodeSpin    = nullptr;
    QLabel*               m_keyNameLabel  = nullptr;
    QPushButton*          m_captureKeyBtn = nullptr;
    QCheckBox*            m_ctrlCheck     = nullptr;
    QCheckBox*            m_shiftCheck    = nullptr;
    QCheckBox*            m_altCheck      = nullptr;
    QCheckBox*            m_winCheck      = nullptr;

    // Страница: TypeText
    QTextEdit*            m_textEdit      = nullptr;
    QSpinBox*             m_typeDelaySpin = nullptr;

    // Страница: Delay
    QSpinBox*             m_delaySpin     = nullptr;

    // Страница: RandomDelay
    QSpinBox*             m_delayMinSpin  = nullptr;
    QSpinBox*             m_delayMaxSpin  = nullptr;

    // Страница: Loop
    QSpinBox*             m_loopCountSpin = nullptr;

    // Страница: SubMacro
    QLineEdit*            m_subMacroEdit  = nullptr;

    // Страница: Comment
    QLineEdit*            m_commentEdit   = nullptr;

    // Страница: MouseMove
    QSpinBox*             m_movePosXSpin  = nullptr;
    QSpinBox*             m_movePosYSpin  = nullptr;

    // Захват клавиши
    bool                  m_capturing     = false;
    int                   m_capturedVk    = 0;

    // Индекс подсвеченного действия
    int                   m_highlightedIndex = -1;

    // Номера страниц параметров
    enum ParamPage {
        PageEmpty = 0,
        PageMouse,
        PageKeyboard,
        PageTypeText,
        PageDelay,
        PageRandomDelay,
        PageLoop,
        PageSubMacro,
        PageComment,
        PageMouseMove
    };

    // Получить номер страницы для типа действия
    ParamPage pageForType(MacroActionType type) const;

protected:
    // Перехватываем нажатия клавиш (для режима захвата)
    bool eventFilter(QObject* obj, QEvent* event) override;
};
