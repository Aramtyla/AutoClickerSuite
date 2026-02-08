# 🎯 AutoClicker Suite

**Advanced AutoClicker Suite** — продвинутый настольный автокликер для Windows с поддержкой мыши, клавиатуры, макросов и интеллектуальных режимов.

Нативное приложение на **C++20 / Qt6 / Win32 API** с минимальным потреблением ресурсов.

[![Build](https://img.shields.io/github/actions/workflow/status/Aramtyla/AutoClickerSuite/build.yml?branch=main&label=Build)](https://github.com/Aramtyla/AutoClickerSuite/actions)
[![Release](https://img.shields.io/github/v/release/Aramtyla/AutoClickerSuite?label=Release)](https://github.com/Aramtyla/AutoClickerSuite/releases/latest)
![Windows](https://img.shields.io/badge/Windows-10%2F11-blue?logo=windows)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-green?logo=qt)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 📥 Скачать

> **[⬇️ Скачать AutoClicker Suite v2.0.0 (установщик, Windows x64)](https://github.com/Aramtyla/AutoClickerSuite/releases/download/v2.0.0/AutoClickerSuite-2.0.0-setup-win64.exe)**
>
> Портативная версия (ZIP): **[AutoClickerSuite-2.0.0-portable-win64.zip](https://github.com/Aramtyla/AutoClickerSuite/releases/download/v2.0.0/AutoClickerSuite-2.0.0-portable-win64.zip)**

---

## 📋 Возможности

### 🖱️ Модуль мыши
- Автоклик ЛКМ / ПКМ / СКМ с интервалом от 1 мс
- Режимы: одиночный, двойной, удержание
- Клик по координатам / следование за курсором
- Мультиточечный маршрут (scripted path)
- Рандомизация интервала и координат (анти-детект)
- Ограничение по количеству или времени

### ⌨️ Модуль клавиатуры
- Автонажатие любой клавиши или комбинации
- Режим зажатия клавиши (удержание до остановки)
- Режим печати текста (строка → посимвольный ввод)
- Запись и воспроизведение клавиатурных макросов
- Поддержка модификаторов (Ctrl, Shift, Alt, Win)
- Рандомизация задержки

### 🔁 Макро-движок
- Визуальный редактор с drag & drop
- Смешанные макросы: мышь + клавиатура + задержки + циклы
- Запись макросов в реальном времени
- Вложенные циклы и подмакросы
- Экспорт / импорт в JSON
- Настраиваемая скорость воспроизведения (0.1x — 10x)

### 🧠 Умные режимы
- Привязка к окну (действия только в указанном окне)
- Поиск и клик по цвету пикселя
- Поиск по изображению (template matching без OpenCV)
- Планировщик задач по расписанию
- Профили: сохранение / загрузка всех настроек

### 🎨 Интерфейс и UX
- Нативный GUI на Qt6 Widgets
- System tray с контекстным меню
- Глобальные хоткеи (F6 — старт/стоп, F7 — запись, F8 — экстренная остановка)
- Настраиваемые горячие клавиши (Ctrl+, → Настройки)
- Тёмная и светлая темы (Catppuccin)
- Лог действий в реальном времени
- Мультиязычность (RU / EN)

---

## 📖 Документация

- [Руководство пользователя (RU)](docs/USER_GUIDE.md)
- [User Guide (EN)](docs/USER_GUIDE_EN.md)

---

## 🛠️ Требования к сборке

| Компонент | Версия |
|-----------|--------|
| **CMake** | 3.24+ |
| **Компилятор** | MSVC 2022 (Visual Studio 17.x) или MinGW 13+ |
| **Qt** | 6.5+ (модули: Core, Widgets, Gui, Network, LinguistTools) |
| **ОС** | Windows 10/11 |

---

## 🚀 Сборка из исходников

### Быстрая сборка

```cmd
:: Клонируем репозиторий
git clone https://github.com/Aramtyla/AutoClickerSuite.git
cd AutoClickerSuite

:: Запускаем скрипт сборки (из Developer Command Prompt)
scripts\build.bat release
```

| Команда | Описание |
|---------|----------|
| `scripts\build.bat` | Release-сборка (по умолчанию) |
| `scripts\build.bat debug` | Debug-сборка |
| `scripts\build.bat release` | Release-сборка |
| `scripts\build.bat portable` | Портативная Release-сборка |
| `scripts\build.bat installer` | Release + NSIS-установщик |
| `scripts\build.bat clean` | Очистка каталога build/ |

### Ручная сборка (CMake)

```powershell
# Настройка (укажите путь к Qt если он не в PATH)
cmake -S . -B build -G Ninja `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2022_64" `
    -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build build --config Release --parallel

# Результат: build/AutoClickerSuite.exe
```

### Портативная сборка

```cmd
scripts\build.bat portable
scripts\package.bat
:: Результат: dist\AutoClickerSuite-1.0.0-portable-win64.zip
```

В портативном режиме настройки сохраняются в `settings.ini` рядом с `.exe` вместо `%APPDATA%`.

### Создание установщика

```cmd
:: Через скрипт сборки
scripts\build.bat installer

:: Или вручную через NSIS (после package.bat)
cd installer
makensis installer.nsi
:: Результат: dist\AutoClickerSuite-1.0.0-setup-win64.exe
```

---

## 📁 Структура проекта

```
AutoClickerSuite/
├── CMakeLists.txt              # Конфигурация сборки
├── README.md                   # Этот файл
├── LICENSE                     # Лицензия MIT
├── scripts/
│   ├── build.bat               # Скрипт сборки
│   └── package.bat             # Упаковка портативного релиза
├── installer/
│   └── installer.nsi           # NSIS-установщик
├── .github/
│   └── workflows/
│       └── build.yml           # CI/CD (GitHub Actions)
├── resources/
│   ├── app.rc                  # Windows-ресурсы (иконка, версия)
│   ├── resources.qrc           # Qt ресурсы
│   ├── icons/                  # Иконки приложения
│   ├── themes/
│   │   ├── dark.qss            # Тёмная тема (Catppuccin Mocha)
│   │   └── light.qss           # Светлая тема (Catppuccin Latte)
│   └── translations/
│       ├── app_en.ts           # Английский перевод
│       └── app_ru.ts           # Русский перевод
└── src/
    ├── main.cpp                # Точка входа
    ├── app/                    # Каркас приложения
    │   ├── MainWindow.*        # Главное окно
    │   ├── TrayManager.*       # Системный трей
    │   ├── HotkeyManager.*     # Глобальные хоткеи (Win32)
    │   ├── ThemeManager.*      # Управление темами
    │   ├── LanguageManager.*   # Мультиязычность
    │   └── ProfileManager.*    # Профили настроек
    ├── core/
    │   └── InputSimulator.*    # Win32 SendInput обёртка
    ├── mouse/                  # Модуль автоклика мыши
    │   ├── MouseWidget.*       # GUI
    │   ├── MouseClicker.*      # Движок
    │   └── MouseConfig.h       # Конфигурация
    ├── keyboard/               # Модуль автоклика клавиатуры
    │   ├── KeyboardWidget.*    # GUI
    │   ├── KeyboardClicker.*   # Движок
    │   └── KeyboardConfig.h    # Конфигурация
    ├── macro/                  # Макро-движок
    │   ├── MacroWidget.*       # GUI
    │   ├── MacroEditor.*       # Визуальный редактор
    │   ├── MacroRecorder.*     # Запись макросов
    │   ├── MacroPlayer.*       # Воспроизведение
    │   └── MacroConfig.*       # Типы действий
    ├── smart/                  # Умные режимы
    │   ├── SmartWidget.*       # GUI
    │   ├── WindowFinder.*      # Привязка к окну
    │   ├── ColorMatcher.*      # Поиск по цвету
    │   ├── ImageMatcher.*      # Поиск по изображению
    │   ├── Scheduler.*         # Планировщик
    │   └── SmartConfig.h       # Конфигурация
    └── utils/                  # Утилиты
        ├── Constants.h         # Глобальные константы
        ├── Logger.*            # Логирование
        └── Settings.*          # Настройки
```

---

## ⌨️ Горячие клавиши

| Комбинация | Действие |
|------------|----------|
| **F6** | Старт / Стоп (мышь, клавиатура, макрос) |
| **F7** | Начать / Остановить запись макроса |
| **F8** | Экстренная остановка всех действий |

Хоткеи работают глобально — даже когда приложение свёрнуто в трей.

---

## 📖 Использование

### Быстрый старт

1. Запустите `AutoClickerSuite.exe`
2. Выберите вкладку нужного модуля (Мышь / Клавиатура / Макросы / Умные режимы)
3. Настройте параметры
4. Нажмите **Старт** или **F6**

### Портативный режим

Скачайте ZIP-архив из [Releases](https://github.com/Aramtyla/AutoClickerSuite/releases). Распакуйте в любую папку и запустите — установка не требуется.

### Запись макроса

1. Перейдите на вкладку **Макросы**
2. Нажмите **F7** или кнопку записи
3. Выполните нужные действия мышью и клавиатурой
4. Нажмите **F7** снова для остановки записи
5. Сохраните макрос и воспроизведите его

---

## ⚙️ Конфигурация

### Темы

Приложение поддерживает тёмную и светлую темы. Переключение: **Вид → Переключить тему**.

Темы основаны на палитре [Catppuccin](https://catppuccin.com/):
- **Тёмная** — Catppuccin Mocha
- **Светлая** — Catppuccin Latte

### Языки

Переключение: **Вид → Переключить язык**. Доступны: русский, английский.

### Профили

**Файл → Загрузить профиль / Сохранить профиль** — сохранение и загрузка всех настроек всех модулей в JSON-файл.

---

## 🔧 Архитектура

```
┌─────────────────────────────────────────┐
│              MainWindow                 │
│  ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │  Mouse   │ │ Keyboard │ │  Macro  │ │
│  │  Widget  │ │  Widget  │ │  Widget │ │
│  └────┬─────┘ └────┬─────┘ └────┬────┘ │
│       │            │            │       │
│  ┌────┴─────┐ ┌────┴─────┐ ┌───┴─────┐ │
│  │  Mouse   │ │ Keyboard │ │ Recorder│ │
│  │  Clicker │ │  Clicker │ │ Player  │ │
│  └────┬─────┘ └────┬─────┘ │ Editor  │ │
│       │            │       └───┬─────┘  │
│       └────────────┴───────────┘        │
│                    │                     │
│           ┌────────┴────────┐            │
│           │ InputSimulator  │            │
│           │   (Win32 API)   │            │
│           └─────────────────┘            │
│                                          │
│  ┌──────────────────────────────────┐    │
│  │         Smart Module             │    │
│  │  WindowFinder | ColorMatcher     │    │
│  │  ImageMatcher | Scheduler        │    │
│  └──────────────────────────────────┘    │
│                                          │
│  ┌──────────────────────────────────┐    │
│  │          Managers                │    │
│  │  Tray | Hotkey | Theme           │    │
│  │  Language | Profile              │    │
│  └──────────────────────────────────┘    │
└──────────────────────────────────────────┘
```

**Паттерны:**
- **Singleton** — Logger, Settings
- **Manager** — Theme, Language, Hotkey, Tray, Profile
- **Widget↔Engine** — GUI и логика разделены через Qt сигналы/слоты
- **Observer** — Qt signal/slot для обновления GUI из движков

---

## 📄 Лицензия

MIT License. Подробности в файле [LICENSE](LICENSE).

---

## 📥 Скачать

### Портативная версия (рекомендуется)
1. Скачайте `AutoClickerSuite-X.X.X-portable-win64.zip` из [Releases](https://github.com/Aramtyla/AutoClickerSuite/releases/latest)
2. Распакуйте в любую папку
3. Запустите `AutoClickerSuite.exe`

### Установщик (NSIS)
1. Скачайте `AutoClickerSuite-X.X.X-setup-win64.exe` из [Releases](https://github.com/Aramtyla/AutoClickerSuite/releases/latest)
2. Запустите и следуйте инструкциям

---

## 📋 Системные требования

| Параметр | Минимум | Рекомендуется |
|----------|---------|---------------|
| **ОС** | Windows 10 (64-бит) | Windows 11 |
| **CPU** | 1 ГГц, 2 ядра | 2+ ГГц |
| **RAM** | 50 МБ | 100 МБ |
| **Диск** | 30 МБ | 50 МБ |

---

## 🤝 Участие в разработке

1. Форкните репозиторий
2. Создайте ветку фичи: `git checkout -b feature/my-feature`
3. Зафиксируйте изменения: `git commit -m "feat: описание"`
4. Отправьте ветку: `git push origin feature/my-feature`
5. Создайте Pull Request

---

## 🙏 Благодарности

- [Qt Project](https://www.qt.io/) — GUI-фреймворк
- [Catppuccin](https://catppuccin.com/) — палитра цветов для тем
- [NSIS](https://nsis.sourceforge.io/) — система создания установщиков
