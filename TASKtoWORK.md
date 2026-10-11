# TASKtoWORK.md — Пошаговый трекер реализации Walking Skeleton

> **Проект:** САПР топологического проектирования СБИС (Команда 1, группа ЭН-44).  
> **Методология:** Walking Skeleton (каждый этап компилируется и верифицируется).  
> **Статусы задач:** `[ ] TODO` (к исполнению), `[!] IN PROGRESS` (в работе), `[x] DONE` (завершено по DoD).  
> **ЖЕСТКОЕ ПРАВИЛО:** Запрещено перескакивать через этапы. Переход к этапу $N+1$ разрешен ТОЛЬКО после 100% готовности этапа $N$.

---

## Этап 0: Базовая инфраструктура проекта и CMake Skeleton
- [ ] **0.1. Создание дерева каталогов:**
  - `include/core/`, `include/io/`, `include/verifier/`
  - `src/core/`, `src/io/`, `src/verifier/`
  - `tests/`
  - `data/` (для эталонных входных JSON)
- [ ] **0.2. Корневой `CMakeLists.txt`:**
  - Стандарт C++17, флаги компилятора MSVC (`/W4 /WX /permissive-`), GCC/Clang (`-Wall -Wextra -Werror`).
  - Подключение `nlohmann/json` (FetchContent или header-only).
  - Настройка CTest (`enable_testing()`).
  - Минимальный исполняемый файл `ga_sp_topology` (stub) и тестовый таргет `test_skeleton`.
- [ ] **0.3. Проверка компиляции и сборки:**
  - `cmake -B build` и `cmake --build build --config Release`.

---

## Этап 1: Фиксация структур данных контрактов (`include/core/` и `include/verifier/`)
- [ ] **1.1. Заголовочные файлы Входного контракта:**
  - `include/core/ChipSpec.hpp`
  - `include/core/BlockData.hpp`
  - `include/core/NetData.hpp`
  - `include/core/GAConfig.hpp`
  - `include/core/InputContext.hpp`
- [ ] **1.2. Заголовочные файлы Процесса ядра (Runtime):**
  - `include/core/Individual.hpp`
  - `include/core/EvaluationScratchpad.hpp`
  - `include/core/PopulationState.hpp`
- [ ] **1.3. Заголовочные файлы Выходного контракта и фундаментальных типов верификатора:**
  - `include/core/PlacedBlock.hpp`
  - `include/core/QualityMetrics.hpp`
  - `include/core/ExecutionStats.hpp`
  - `include/verifier/VerificationReport.hpp` (структуры нарушений и отчёта, устраняет C1083 для PlacementResult)
  - `include/core/PlacementResult.hpp`
- [ ] **1.4. Проверка компилируемости заголовков:**
  - Создание фиктивного `tests/test_data_formats.cpp` с включением всех заголовков ядра и отчёта верификатора, компиляция в Release и Debug.

---

## Этап 2: Слой IO и Независимый Верификатор (TDD фундамент)
- [ ] **2.1. Независимый `verifier::PlacementVerifier`:**
  - Заголовочный файл `include/verifier/PlacementVerifier.hpp`.
  - Реализация `src/verifier/VerificationReport.cpp` (метод `to_string()`) и `src/verifier/PlacementVerifier.cpp` ($O(N^2)$ AABB пересечения, рамка чипа, сетка, проверка HPWL).
  - Юнит-тесты `tests/test_verifier.cpp` (позитивные и негативные тесты на искусственные коллизии).
- [ ] **2.2. Модуль `io::JsonParser` и `io::JsonWriter`:**
  - `include/io/JsonIO.hpp`, реализация `src/io/JsonIO.cpp`.
  - Построение двусторонней `NameTable`.
  - Поддержка канонического ключа `"memory_mb"` и псевдонима `"peak_memory_mb"` в телеметрии.
  - Юнит-тесты `tests/test_json_io.cpp` (парсинг `data/sample_team1.json`, обработка битого JSON, проверка схемы вывода).

---

## Этап 3: Геометрический декодер Sequence Pair (`IPlacementDecoder`)
- [ ] **3.1. Интерфейс `IPlacementDecoder`:**
  - `include/core/IPlacementDecoder.hpp`.
- [ ] **3.2. Эталонная реализация `DagPlacementDecoder` ($O(N^2)$):**
  - `include/core/DagPlacementDecoder.hpp`, `src/core/DagPlacementDecoder.cpp`.
  - Hot Loop метод `decode_to_scratchpad` с записью в предвыделенный `EvaluationScratchpad`.
  - Метод материализации `decode_to_layout`.
- [ ] **3.3. Тестирование декодера:**
  - `tests/test_placement_decoder.cpp`:
    - Аналитический тест на 3 блоках (`Test_Analytical_3_Blocks_No_Rotation`).
    - Проверка ориентации при `is_rotated == 1`.
    - Тест на отсутствие аллокаций в куче в цикле из 10 000 итераций.

---

## Этап 4: Сервис оценки фитнес-функции (`ICostEvaluator`)
- [ ] **4.1. Интерфейс `ICostEvaluator`:**
  - `include/core/ICostEvaluator.hpp`.
- [ ] **4.2. Реализация `StandardCostEvaluator`:**
  - `include/core/StandardCostEvaluator.hpp`, `src/core/StandardCostEvaluator.cpp`.
  - Расчет HPWL строго по геометрическим центрам блоков ($O(\sum |\text{net}|)$).
  - Расчет базовой нормализации `Area_norm` и `HPWL_norm`.
  - Квадратичный штраф за выход за пределы кристалла (`Penalty`).
- [ ] **4.3. Тестирование оценщика:**
  - `tests/test_cost_evaluator.cpp`:
    - Аналитический тест расчета HPWL для 2 блоков.
    - Обработка вырожденных сетей (1 блок, пустая сеть).
    - Проверка начисления штрафа при превышении границ.

---

## Этап 5: Движок генетического алгоритма (`IGAEngine` / `GARunner`)
- [ ] **5.1. Интерфейс и класс `GARunner`:**
  - `include/core/IGAEngine.hpp`, `src/core/GARunner.cpp`.
- [ ] **5.2. Операторы ГА:**
  - `tournament_select` (турнирная селекция).
  - `order_crossover` (OX) и `pmx_crossover` (PMX) с проверкой биективности перестановок.
  - `mutate_swap` и `mutate_rotate` (с учетом блокировки `rotation_allowed`).
  - Элитизм и двойная буферизация (`PopulationState::next_generation`).
- [ ] **5.3. Тестирование операторов ГА:**
  - `tests/test_ga_engine.cpp`:
    - Проверка сохранения биективности перестановок после OX и PMX.
    - Проверка соблюдения запрета на поворот при `rotation_allowed = false`.
    - Проверка монотонности истории лучшего фитнеса (элитизм).

---

## Этап 6: Интеграция CLI, Профилирование и Сквозной прогон
- [ ] **6.1. Главная точка входа (`src/main.cpp`):**
  - Чтение аргументов командной строки (`--input <path>`, `--output <path>`, `--seed <val>`).
  - Замер времени выполнения (`std::chrono::high_resolution_clock`).
  - Замер пикового потребления памяти через Windows API (`psapi.h`, `GetProcessMemoryInfo`).
  - Финальный прогон через `verifier::PlacementVerifier` перед сохранением результата.
- [ ] **6.2. Сквозное E2E тестирование:**
  - Прогон на легком тесте (10 блоков), среднем (100 блоков) и стресс-тесте (500–1000 блоков).
  - Проверка соответствия выходного JSON спецификации.
