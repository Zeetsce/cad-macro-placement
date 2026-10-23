# Руководство пользователя и разработчика»

> **Проект:** САПР двумерного топологического размещения прямоугольных макроблоков на кристалле методом генетического алгоритма на базе парных последовательностей (Sequence Pair).
> **Авторы:** Команда 1 (группа ЭН-44)
> **Стек:** C++17, CMake 3.20+, MSVC / GCC / Clang, Ninja / MSBuild, OpenMP, `nlohmann/json`.

---

## Содержание
1. [Установка зависимостей и настройка окружения (PATH)](#1-установка-зависимостей-и-настройка-окружения-path)
2. [Сборка проекта](#2-сборка-проекта)
3. [Запуск и параметры командной строки](#3-запуск-и-параметры-командной-строки)
4. [Входные данные (спецификация JSON)](#4-входные-данные-спецификация-json)
5. [Выходные данные (результаты и топология)](#5-выходные-данные-результаты-и-топология)
6. [Замер памяти и времени выполнения](#6-замер-памяти-и-времени-выполнения)
7. [Тестирование и независимый верификатор](#7-тестирование-и-независимый-верификатор)

---

## 1. Установка зависимостей и настройка окружения (PATH)

Для сборки и работы САПР требуются:
- **Компилятор C++17:** MSVC (Visual Studio 2022 v143+), Clang 10+ или GCC 9+.
- **Система сборки CMake:** версия 3.20 или новее.
- **Генератор сборки Ninja:** для параллельной и быстрой компиляции.
- **Git:** для загрузки библиотеки `nlohmann/json` через механизм CMake FetchContent.

### 1.1. Установка через пакетные менеджеры

#### Вариант А: Winget (Windows 10/11)
Выполните в консоли PowerShell от имени администратора:

```powershell
# Установка Git
winget install --id Git.Git -e --source winget

# После установки укажите данные для коммитов
git config --global user.name "Ваше Имя"
git config --global user.email "your.email@example.com"

# Установка CMake с автоматическим добавлением в системный PATH
winget install --id Kitware.CMake -e --override "ADD_CMAKE_TO_PATH=System"

# Установка Ninja
winget install --id Ninja-build.Ninja -e

# Установка компилятора C++ (Visual Studio 2022 Build Tools с компонентами C++)
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```


#### Вариант Б: Scoop
```powershell
# Установка SCOOP
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression

# Установка Компонентов через SCOOP
scoop install git cmake ninja
```
Если разработка ведется с использованием GCC или Clang вместо MSVC:
1. Установите MSYS2: `winget install MSYS2.MSYS2 -e`
2. В консоли MSYS2 установите тулчейн (GCC или Clang, CMake, Ninja):
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   pacman -S mingw-w64-ucrt-x86_64-clang
   pacman -S mingw-w64-ucrt-x86_64-cmake
   pacman -S mingw-w64-ucrt-x86_64-ninja
   ```
3. Добавьте путь к бинарным файлам UCRT64 в пользовательский PATH Windows:
   ```powershell
   [System.Environment]::SetEnvironmentVariable("Path", [System.Environment]::GetEnvironmentVariable("Path","User") + ";C:\msys64\ucrt64\bin", "User")
   ```

---

### 1.2. Контрольная проверка доступности инструментов

Перед сборкой убедитесь, что все компоненты доступны:
```powershell
cmake --version
ninja --version
cl    # При использовании MSVC
# или: gcc --version (при использовании MinGW)
```

---

### 1.3. Часто встречающиеся проблемы

При установке утилит на Windows часто возникает ситуация, когда установленные программы (`cmake`, `ninja`, `cl.exe`, `gcc`) не распознаются терминалом.

#### Проблема 1: Отсутствие компилятора MSVC (`cl.exe`) в стандартном PowerShell
Компилятор MSVC по умолчанию **не добавляется в глобальный PATH**. Запуск сборки из обычной сессии PowerShell завершится ошибкой ненахождения C++ компилятора.

**Решения:**
1. **Рекомендуемое:** запускать терминал **«Developer PowerShell for VS 2022»** (или «Developer Command Prompt for VS 2022») из меню «Пуск».
2. **Либо активировать среду MSVC в текущей консоли PowerShell:**
   ```powershell
   # Для Visual Studio Build Tools:
   & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1"

   # Для Visual Studio Community / Professional:
   & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"
   ```

#### Проблема 2: CMake или Ninja не найдены в активной сессии
После установки через Winget открытый терминал не получает обновленные переменные среды Windows.

**Решение для текущей сессии:**
```powershell
# Обновление переменной PATH текущего процесса из реестра без перезапуска PowerShell:
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
```

Если утилита установлена в нестандартный каталог (например, ручная распаковка `ninja.exe`):
```powershell
# Добавление пути в пользовательский PATH на постоянной основе:
[System.Environment]::SetEnvironmentVariable("Path", [System.Environment]::GetEnvironmentVariable("Path","User") + ";C:\Tools\Ninja", "User")
```

#### Проблема 3: Использование GCC / Clang (MinGW-w64 / MSYS2 UCRT64)
Если разработка ведется с использованием GCC или Clang вместо MSVC:
1. Установите MSYS2: `winget install MSYS2.MSYS2 -e`
2. В консоли MSYS2 установите тулчейн (GCC или Clang, CMake, Ninja):
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```
3. Добавьте путь к бинарным файлам UCRT64 в пользовательский PATH Windows:
   ```powershell
   [System.Environment]::SetEnvironmentVariable("Path", [System.Environment]::GetEnvironmentVariable("Path","User") + ";C:\msys64\ucrt64\bin", "User")
   ```

#### Проблема 4: Ошибка сборки через расширение VS Code CMake Tools (`clang-cl exited with code 1`)
Если в VS Code установлено расширение **CMake Tools**, при вызове конфигурации (`Ctrl+Shift+P -> CMake: Configure`) может возникнуть ошибка, если расширение автоматически выбрало Kit `clang-cl`:
- `clang-cl` — это интерфейс Clang, ориентированный на среду Microsoft Visual Studio (`libcmt.lib`). Без установленного Visual Studio C++ Build Tools линковщик завершится с ошибкой.
- **Решение:** Нажмите в нижней строке состояния VS Code на имя текущего набора инструментов (Kit) либо вызовите `Ctrl+Shift+P` -> `CMake: Select a Kit` и выберите автономный тулчейн MSYS2:
  - **`Clang ... x86_64-w64-windows-gnu` (ucrt64)**
  - либо **`GCC ... x86_64-w64-mingw32` (ucrt64)**.
После этого конфигурация и сборка по кнопке `Build` (или `F7`) пройдут штатно.

---

## 2. Сборка проекта

Сборка осуществляется кроссплатформенным генератором CMake со строгими проверками компилятора (`/W4 /WX` в MSVC, `-Wall -Wextra -Wpedantic -Werror` в GCC/Clang).

```powershell
# 1. Конфигурация в режиме Release с генератором Ninja
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -GNinja

# 2. Компиляция всех компонентов (ядро, CLI, верификатор, тесты)
cmake --build build --config Release
```

Если используется стандартный генератор Visual Studio:
```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Расположение исполняемого файла:
- При сборке через Ninja: `build/ga_sp_topology.exe`
- При сборке через Visual Studio generator: `build/Release/ga_sp_topology.exe`

---

## 3. Запуск и параметры командной строки

### Синтаксис вызова
```powershell
.\build\ga_sp_topology.exe [опции]
```

### Доступные параметры CLI
| Флаг | Тип | По умолчанию | Описание |
| :--- | :--- | :--- | :--- |
| `--input <path>` | путь к файлу | `data/sample_team1.json` | Путь к входному JSON-файлу проекта с описанием чипа, блоков и цепей. |
| `--output <path>` | путь к файлу | `output.json` | Путь для сохранения выходного JSON с координатами и метриками. |
| `--svg <path>` | путь к файлу | `placement.svg` | Опциональный путь к файлу векторной графики для отрисовки чертежа. |
| `--seed <uint>` | целое число | `42` | Начальное зерно ПСЧ для детерминизма и воспроизводимости стохастической оптимизации. |
| `--help`, `-h` | флаг | — | Вывод справки по доступным параметрам. |

### Примеры запуска

**Пример 1: Базовый запуск на тестовом чипе с генерацией схемы**
```powershell
.\build\ga_sp_topology.exe --input data/sample_team1.json --output out_sample.json --svg out_sample.svg
```

**Пример 2: Запуск оптимизации сложного кристалла (600 блоков) с заданным seed**
```powershell
.\build\ga_sp_topology.exe --input data/test_hard_600.json --output out_hard.json --svg out_hard.svg --seed 1337
```

---

## 4. Входные данные (спецификация JSON)

Входной файл задает физические ограничения кристалла, перечень размещаемых элементов, гиперребра электрических цепей и настройки генетического алгоритма.

Готовые тестовые файлы в каталоге `data/`:
- `data/sample_team1.json` — минимальный демонстрационный пример (3 блока, 2 цепи).
- `data/test_easy_15.json` — 15 макроблоков, базовые соединения.
- `data/test_medium_150.json` — 150 макроблоков, 60 цепей.
- `data/test_hard_600.json` — 600 макроблоков, 250 цепей.
- `data/test_dense_1000.json` — стресс-тест: 1000 макроблоков, плотность заполнения площади кристалла свыше 80%.

### Схема входного файла

```json
{
  "design": {
    "chip_width": 1000.0,       // Максимально допустимая ширина кристалла
    "chip_height": 800.0,       // Максимально допустимая высота кристалла
    "grid_size": 10.0,          // Шаг координатной сетки (все координаты квантуются кратно grid_size)
    "rotation_allowed": true    // Глобальное разрешение поворота компонентов на 90 градусов
  },
  "blocks": [
    {
      "id": "Cell1",            // Уникальный строковый идентификатор макроблока
      "width": 120.0,           // Геометрическая ширина блока
      "height": 80.0,           // Геометрическая высота блока
      "rotation_allowed": true, // Локальный флаг разрешения поворота
      "weight": 1.0             // Весовой приоритет блока при расчете целевой функции
    },
    {
      "id": "Cell2",
      "width": 150.0,
      "height": 90.0,
      "rotation_allowed": true,
      "weight": 1.2
    }
  ],
  "nets": [
    {
      "net_id": "N1",                    // Идентификатор электрической цепи (гиперребра)
      "block_ids": ["Cell1", "Cell2"]    // Список ID блоков, объединяемых данной цепью
    }
  ],
  "ga_params": {
    "population_size": 60,               // Размер популяции особей
    "generations": 120,                  // Лимит эпох эволюции
    "crossover_type": "sequence_pair",   // Тип кроссовера ("sequence_pair", "ox", "pmx")
    "selection_method": "tournament",    // Метод селекции родителей
    "tournament_size": 4,                // Размер турнирной группы
    "crossover_probability": 0.85,       // Вероятность скрещивания пары
    "mutation_probability": 0.12,        // Вероятность оператора мутации
    "elit_count": 3,                     // Число лучших особей, переходящих в следующее поколение без изменений
    "fitness_weights": {
      "hpwl": 0.65,                      // Весовой коэффициент минимизации суммарной длины соединений
      "area": 0.35                       // Весовой коэффициент минимизации площади описывающего прямоугольника
    }
  }
}
```

---

## 5. Выходные данные (результаты и топология)

### 1. Формат выходного JSON (`output.json`)
Выходной файл содержит координаты размещения и телеметрию выполнения:

```json
{
  "hpwl": 220.0,                        // Итоговая суммарная полупериметровая длина связей
  "area": 34500.0,                      // Площадь описывающего прямоугольника
  "blocks": [                           // Список размещенных компонентов
    {
      "id": "Cell1",
      "x": 0.0,                         // Координата X левого нижнего угла (кратна grid_size)
      "y": 150.0,                       // Координата Y левого нижнего угла (кратна grid_size)
      "width": 120.0,                   // Результирующая ширина с учетом поворота
      "height": 80.0,                   // Результирующая высота с учетом поворота
      "rotated": false                  // Флаг поворота на 90 градусов
    }
  ],
  "placement": [ ... ],                 // Дублирующий массив для совместимости
  "fitness_history": [                  // Динамика лучшего значения функции приспособленности
    0.145, 0.120, 0.0945
  ],
  "metrics": {
    "actual_width": 150.0,              // Габаритная ширина размещения
    "actual_height": 230.0,             // Габаритная высота размещения
    "total_area": 34500.0,              // Суммарная площадь охватывающего бокса
    "total_hpwl": 220.0,                // Взвешенный HPWL
    "chip_utilization": 0.8087,         // Коэффициент полезного заполнения площади
    "final_cost": 0.0945                // Значение комплексной фитнес-функции
  },
  "telemetry": {
    "runtime_ms": 13,                   // Время расчета ГА в миллисекундах
    "memory_mb": 5.50,                  // Пиковое потребление оперативной памяти процессом (в МБ)
    "convergence_generation": 0,        // Номер поколения стабилизации
    "generations_completed": 120        // Количество пройденных эпох
  }
}
```

### 2. Векторный чертеж SVG (`placement.svg`)
При указании ключа `--svg <path>` формируется масштабируемый векторный чертеж:
- Внешняя рамка кристалла `chip_width` x `chip_height`.
- Дискретная координатная сетка с шагом `grid_size`.
- Прямоугольники макроблоков с заливкой, идентификатором и геометрическими размерами.
- Манхэттенские связи (гиперребра) между центрами блоков.

---

## 6. Замер памяти и времени выполнения

### Встроенная телеметрия процесса
1. **Пиковая оперативная память (RAM):**
   - На платформе Windows сбор телеметрии реализован через Win32 API (`GetProcessMemoryInfo` из `<psapi.h>`).
   - Фиксируется точное значение `PeakWorkingSetSize`, которое выводится в консоль и записывается в `output.json` под ключом `telemetry.memory_mb`.
2. **Zero-Allocation Hot Loop:**
   - Алгоритм удовлетворяет жесткому инварианту: отсутствие динамических выделений памяти в критическом эволюционном цикле ($N_{\text{pop}} \times N_{\text{gen}}$).
   - Вся память под декодирование DAG-графов и расчет фитнеса предвыделяется однократно в объекте `EvaluationScratchpad` до старта эволюции.
3. **Чистое процессорное время (Runtime):**
   - Замеряется высокоточным таймером `std::chrono::high_resolution_clock` и фиксирует только расчет ядра (без учета дискового ввода-вывода).

### Внешний замер системными средствами PowerShell
Для независимой оценки полного времени выполнения программы (включая инициализацию и ввод-вывод):
```powershell
Measure-Command { .\build\ga_sp_topology.exe --input data/test_medium_150.json }
```

Для замера пиковой памяти через процесс PowerShell:
```powershell
$proc = Start-Process .\build\ga_sp_topology.exe -ArgumentList "--input data/test_hard_600.json" -PassThru
$proc.WaitForExit()
Write-Host ("Затраченная память (Peak RAM): {0:N2} MB" -f ($proc.PeakWorkingSet64 / 1MB))
```

---

## 7. Тестирование и независимый верификатор

### Запуск CTest с замером времени тестов
Набор тестов включает 7 таргетов:

```powershell
# Запуск тестов с выводом диагностических сообщений при ошибках
ctest --test-dir build -C Release --output-on-failure

# Детальный запуск тестов с отображением времени прохождения каждого теста (в секундах)
ctest --test-dir build -C Release --verbose
```

### Запуск выборочного теста
```powershell
# Запуск только тестов генетического алгоритма
ctest --test-dir build -R test_ga_engine --verbose

# Либо прямой запуск исполняемого файла теста
.\build\test_ga_engine.exe
```

### Независимый верификатор `PlacementVerifier`
После завершения работы генетического алгоритма и перед сохранением на диск результат проходит проверку независимым модулем верификации:
1. **Проверка коллизий (O(N^2) AABB):** гарантирует отсутствие геометрических пересечений макроблоков.
2. **Проверка границ:** гарантирует нахождение всех блоков строго внутри контура кристалла.
3. **Проверка сетки:** гарантирует кратность координат $(x, y)$ шагу `grid_size`.
4. **Аудит HPWL:** независимый пересчет полупериметра цепей и сверка с метрикой алгоритма.

При обнаружении нарушений программа выводит детальный отчет верификатора и возвращает ненулевой код ошибки `EXIT_FAILURE`.
