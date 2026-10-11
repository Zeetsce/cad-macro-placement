# SPEC_DATA_FORMATS.md — Спецификация структур данных и контрактов

> **Проект:** САПР топологического проектирования СБИС (Команда 1, группа ЭН-44).  
> **Стандарт:** C++17, ISO/IEC 14882:2017.  
> **Инварианты:** Immutability of Input, Zero-Allocation Runtime, Strict Typing, Bijective NameTable.

---

## 1. Архитектурная модель разделения данных

Система использует двухуровневую модель данных для достижения максимальной производительности (лимит $\le 300$ с на 1000 блоков) при строгом соблюдении JSON-формата ТЗ:

```
    [Входной JSON на диске]
               │
               ▼
        io::JsonParser  (Десериализация + построение NameTable)
               │
               ▼
     core::InputContext  (Иммутабельный контекст: плоские массивы, индексы 0..N-1)
               │
               ▼
     core::GARunner / core::IPlacementDecoder  (Zero-Allocation Hot Loop через Scratchpad)
               │
               ▼
    core::PlacementResult  (Финальное размещение, метрики, история фитнеса)
               │
               ▼
        io::JsonWriter  (Сериализация в JSON с обратной подстановкой имён)
               │
               ▼
    [Выходной JSON на диске]
```

---

## 2. Структуры данных C++17: Входной контракт (`include/core/`)

### 2.1. Спецификация кристалла: `ChipSpec` (`include/core/ChipSpec.hpp`)
```cpp
#pragma once

namespace core {

/**
 * @brief Параметры кристалла и глобальные технологические ограничения.
 */
struct ChipSpec {
    double width = 0.0;            ///< Допустимая ширина кристалла (chip_width > 0)
    double height = 0.0;           ///< Допустимая высота кристалла (chip_height > 0)
    double grid_size = 1.0;        ///< Шаг сетки трассировки/размещения (grid_size > 0)
    bool rotation_allowed = false; ///< Master-флаг разрешения поворота на кристалле
};

} // namespace core
```

**Инварианты и предусловия `ChipSpec`:**
- `width > 0.0` и `height > 0.0` (кристалл физически существует).
- `grid_size > 0.0` (исключает деление на ноль при выравнивании).
- `width >= grid_size` и `height >= grid_size`.
- **Master/Slave правило поворота:** если `ChipSpec::rotation_allowed == false`, ни один блок не может быть повёрнут, даже если его индивидуальный флаг равен `true`.

---

### 2.2. Физический блок: `BlockData` (`include/core/BlockData.hpp`)
```cpp
#pragma once

namespace core {

/**
 * @brief Неизменяемые технологические характеристики макроблока.
 */
struct BlockData {
    int id = -1;                   ///< Уникальный плоский индекс [0 .. N-1]
    double width = 0.0;            ///< Номинальная ширина блока (width > 0)
    double height = 0.0;           ///< Номинальная высота блока (height > 0)
    double weight = 1.0;           ///< Весовой коэффициент блока в целевой функции (> 0)
    bool rotation_allowed = false; ///< Разрешение на поворот данного конкретного блока
};

} // namespace core
```

**Инварианты и предусловия `BlockData`:**
- `id >= 0` и уникален в пределах `InputContext`.
- `width > 0.0`, `height > 0.0`, `weight > 0.0`.
- **Проверка вместимости:** блок обязан помещаться на кристалл:
  - При `rotation_allowed == true`: $\min(w, h) \le \min(W_{\text{chip}}, H_{\text{chip}}) \land ((w \le W \land h \le H) \lor (h \le W \land w \le H))$.
  - При `rotation_allowed == false`: $w \le W_{\text{chip}} \land h \le H_{\text{chip}}$.

---

### 2.3. Электрическая цепь: `NetData` (`include/core/NetData.hpp`)
```cpp
#pragma once

#include <vector>

namespace core {

/**
 * @brief Гиперребро схемы (список связываемых блоков).
 */
struct NetData {
    int id = -1;                          ///< Индекс цепи [0 .. M-1]
    std::vector<int> connected_block_ids; ///< Индексы подключенных блоков [0 .. N-1]
};

} // namespace core
```

**Инварианты и краевые случаи `NetData`:**
- Каждый индекс в `connected_block_ids` обязан удовлетворять $0 \le \text{idx} < N$.
- **Вырожденные цепи:** $\vert\text{connected\_block\_ids}\vert \le 1$ дают вклад в HPWL, равный строго $0.0$.
- **Пустой список цепей:** $M = 0$ допустим; в этом случае целевая функция оптимизирует исключительно площадь кристалла.

---

### 2.4. Параметры генетического алгоритма: `GAConfig` (`include/core/GAConfig.hpp`)
```cpp
#pragma once

namespace core {

/**
 * @brief Метод селекции родителей.
 */
enum class SelectionMethod {
    TOURNAMENT, ///< Турнирная селекция (по умолчанию)
    ROULETTE    ///< Рулеточная селекция
};

/**
 * @brief Тип кроссовера перестановок Sequence Pair.
 */
enum class CrossoverType {
    ORDER_CROSSOVER,           ///< OX (Order Crossover)
    PARTIALLY_MAPPED_CROSSOVER ///< PMX (Partially Mapped Crossover)
};

/**
 * @brief Конфигурация эволюционного поиска.
 */
struct GAConfig {
    int population_size = 50;       ///< Размер популяции N_pop >= 4
    int generations = 100;          ///< Число поколений N_gen >= 1
    int tournament_size = 3;        ///< Размер турнирной группы 2 <= K <= N_pop
    int elit_count = 2;             ///< Количество элитных особей 0 <= elit < N_pop
    
    double crossover_prob = 0.8;    ///< Вероятность скрещивания [0.0, 1.0]
    double mutation_prob = 0.15;    ///< Вероятность мутации [0.0, 1.0]
    
    double hpwl_weight = 0.7;       ///< Вес HPWL в целевой функции (>= 0.0)
    double area_weight = 0.3;       ///< Вес площади в целевой функции (>= 0.0)
    
    SelectionMethod selection = SelectionMethod::TOURNAMENT;
    CrossoverType crossover = CrossoverType::ORDER_CROSSOVER;
};

} // namespace core
```

---

### 2.5. Консолидированный входной контекст: `InputContext` (`include/core/InputContext.hpp`)
```cpp
#pragma once

#include "core/ChipSpec.hpp"
#include "core/BlockData.hpp"
#include "core/NetData.hpp"
#include "core/GAConfig.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace core {

/**
 * @brief Полный неизменяемый контекст задачи оптимизации.
 */
struct InputContext {
    ChipSpec chip;
    std::vector<BlockData> blocks;
    std::vector<NetData> nets;
    GAConfig ga_config;

    // Двусторонняя таблица имён (NameTable) для IO
    std::vector<std::string> index_to_block_name;        ///< id -> "Cell1"
    std::unordered_map<std::string, int> block_name_to_index; ///< "Cell1" -> id
    std::vector<std::string> index_to_net_name;          ///< id -> "N1"
};

} // namespace core
```

---

## 3. Структуры данных C++17: Внутренний процесс ядра (`Runtime State`)

### 3.1. Генотип особи: `Individual` (`include/core/Individual.hpp`)
```cpp
#pragma once

#include <vector>
#include <cstdint>
#include <limits>

namespace core {

/**
 * @brief Генотип особи в представлении Sequence Pair.
 * @note Память минимизирована: фенотипические координаты не хранятся во избежание деградации CPU-кэша.
 */
struct Individual {
    std::vector<int> gamma_plus;   ///< Первая перестановка Sequence Pair (размер N)
    std::vector<int> gamma_minus;  ///< Вторая перестановка Sequence Pair (размер N)
    
    /// Флаги поворота (std::vector<uint8_t> вместо vector<bool> для потокобезопасности и скорости)
    std::vector<uint8_t> is_rotated;

    double fitness = std::numeric_limits<double>::infinity(); ///< Минимизируемый фитнес-скор
    double raw_hpwl = 0.0;   ///< Абсолютный HPWL (кэш для отчёта)
    double raw_area = 0.0;   ///< Абсолютная площадь (кэш для отчёта)
    bool is_feasible = true; ///< Флаг непревышения границ кристалла
};

} // namespace core
```

**Инварианты `Individual`:**
- `gamma_plus.size() == gamma_minus.size() == is_rotated.size() == N`.
- `gamma_plus` и `gamma_minus` являются строгими биекциями чисел $\{0, 1, \dots, N-1\}$.
- Если `!chip.rotation_allowed || !blocks[i].rotation_allowed`, то `is_rotated[i] == 0` строго всегда.

---

### 3.2. Буфер вычислений: `EvaluationScratchpad` (`include/core/EvaluationScratchpad.hpp`)
```cpp
#pragma once

#include <vector>
#include <cstddef>

namespace core {

/**
 * @brief Предвыделенный плоский буфер для декодирования и оценки без динамических аллокаций.
 * @note Выделяется 1 экземпляр на рабочий поток (thread-local).
 */
struct EvaluationScratchpad {
    std::vector<double> x;        ///< Координаты x левых нижних углов
    std::vector<double> y;        ///< Координаты y левых нижних углов
    std::vector<double> w;        ///< Фактические ширины с учётом поворота
    std::vector<double> h;        ///< Фактические высоты с учётом поворота
    
    std::vector<double> center_x; ///< Предвычисленные центры x_c = x + w / 2
    std::vector<double> center_y; ///< Предвычисленные центры y_c = y + h / 2

    // Вспомогательные буферы для O(N^2) DAG / Longest Path
    std::vector<int> position_in_minus; ///< Индекс блока в gamma_minus
    std::vector<double> length_buffer;  ///< Динамическое программирование длиннейшего пути

    void resize(size_t block_count) {
        x.resize(block_count);
        y.resize(block_count);
        w.resize(block_count);
        h.resize(block_count);
        center_x.resize(block_count);
        center_y.resize(block_count);
        position_in_minus.resize(block_count);
        length_buffer.resize(block_count);
    }
};

} // namespace core
```

---

### 3.3. Состояние популяции: `PopulationState` (`include/core/PopulationState.hpp`)
```cpp
#pragma once

#include "core/Individual.hpp"
#include <vector>

namespace core {

/**
 * @brief Буфер поколений с поддержкой Double Buffering без переаллокаций.
 */
struct PopulationState {
    int current_generation = 0;
    
    std::vector<Individual> individuals;     ///< Текущее поколение
    std::vector<Individual> next_generation; ///< Буфер для потомков (меняются через std::swap)
    
    Individual best_ever;                    ///< Абсолютно лучшая особь
    std::vector<double> best_fitness_history;///< История лучшего фитнеса [0 .. N_gen]
};

} // namespace core
```

---

## 4. Структуры данных C++17: Выходной контракт (`Output Result`)

### 4.1. Размещённый блок: `PlacedBlock` (`include/core/PlacedBlock.hpp`)
```cpp
#pragma once

namespace core {

/**
 * @brief Физически материализованный блок на топологическом чертеже.
 */
struct PlacedBlock {
    int block_id = -1;       ///< Индекс блока [0 .. N-1]
    
    double x = 0.0;          ///< Левый нижний угол: координата X
    double y = 0.0;          ///< Левый нижний угол: координата Y
    double width = 0.0;      ///< Фактическая ширина (с учетом поворота)
    double height = 0.0;     ///< Фактическая высота (с учетом поворота)
    
    double center_x = 0.0;   ///< Геометрический центр: X (строго для HPWL)
    double center_y = 0.0;   ///< Геометрический центр: Y (строго для HPWL)
    
    bool is_rotated = false; ///< Фактическое состояние поворота
};

} // namespace core
```

---

### 4.2. Метрики качества: `QualityMetrics` (`include/core/QualityMetrics.hpp`)
```cpp
#pragma once

namespace core {

/**
 * @brief Физические метрики качества итогового размещения.
 */
struct QualityMetrics {
    double total_hpwl = 0.0;       ///< Суммарный полупериметр связей (HPWL)
    double total_area = 0.0;       ///< Площадь охватывающего прямоугольника W_actual * H_actual
    double actual_width = 0.0;     ///< max(x_i + w_i)
    double actual_height = 0.0;    ///< max(y_i + h_i)
    double chip_utilization = 0.0; ///< Суммарная площадь блоков / total_area [0.0 .. 1.0]
    double final_cost = 0.0;       ///< Значение взвешенной целевой функции
};

} // namespace core
```

---

### 4.3. Телеметрия выполнения: `ExecutionStats` (`include/core/ExecutionStats.hpp`)
```cpp
#pragma once

#include <cstdint>

namespace core {

/**
 * @brief Профилирование времени, памяти и сходимости для отчёта по ГОСТ.
 * @note В коде C++ используется семантически точное имя peak_memory_mb.
 * В выходной JSON по спецификации ТЗ сериализуется канонический ключ "memory_mb".
 */
struct ExecutionStats {
    int64_t elapsed_time_ms = 0;     ///< Время работы в миллисекундах (chrono)
    double peak_memory_mb = 0.0;     ///< Пиковая оперативная память в МБ (WinAPI GetProcessMemoryInfo)
    int total_generations = 0;       ///< Пройдено поколений
    int convergence_generation = -1; ///< Поколение, на котором наступила стабилизация
};

} // namespace core
```

---

### 4.4. Структура отчёта верификации: `VerificationReport` (`include/verifier/VerificationReport.hpp`)
```cpp
#pragma once

#include <vector>
#include <string>

namespace verifier {

struct OverlapViolation {
    int block_a = -1;
    int block_b = -1;
    double intersection_area = 0.0;
};

struct BoundaryViolation {
    int block_id = -1;
    double overflow_x = 0.0;
    double overflow_y = 0.0;
};

/**
 * @brief Базовые типы отчёта верификации (создаются на Этапе 1 как фундаментальный контракт).
 */
struct VerificationReport {
    bool is_valid = false;
    std::vector<OverlapViolation> overlaps;
    std::vector<BoundaryViolation> boundary_escapes;
    std::vector<int> grid_violations;
    std::vector<int> illegal_rotations;
    std::vector<int> missing_or_duplicate_blocks;
    double recalculated_hpwl = 0.0;
    bool hpwl_matches = false;

    std::string to_string() const;
};

} // namespace verifier
```

---

### 4.5. Консолидированный результат: `PlacementResult` (`include/core/PlacementResult.hpp`)
```cpp
#pragma once

#include "core/PlacedBlock.hpp"
#include "core/QualityMetrics.hpp"
#include "core/ExecutionStats.hpp"
#include "verifier/VerificationReport.hpp"

#include <vector>

namespace core {

/**
 * @brief Полный агрегированный результат работы САПР.
 */
struct PlacementResult {
    std::vector<PlacedBlock> blocks;
    QualityMetrics metrics;
    ExecutionStats stats;
    std::vector<double> fitness_history;
    verifier::VerificationReport verification;
};

} // namespace core
```

---

## 5. Форматы файлов JSON

### 5.1. Схема входного файла JSON (`input.json`)
```json
{
  "design": {
    "chip_width": 1000.0,
    "chip_height": 800.0,
    "grid_size": 10.0,
    "rotation_allowed": true
  },
  "blocks": [
    {
      "id": "Cell1",
      "width": 120.0,
      "height": 80.0,
      "rotation_allowed": true,
      "weight": 1.0
    }
  ],
  "nets": [
    {
      "net_id": "N1",
      "block_ids": ["Cell1", "Cell2"]
    }
  ],
  "ga_params": {
    "population_size": 50,
    "generations": 100,
    "crossover_type": "sequence_pair",
    "mutation_types": ["swap", "rotate"],
    "selection_method": "tournament",
    "tournament_size": 3,
    "crossover_probability": 0.8,
    "mutation_probability": 0.15,
    "elit_count": 2,
    "fitness_weights": {
      "hpwl": 0.7,
      "area": 0.3
    }
  }
}
```

### 5.2. Схема выходного файла JSON (`output.json`)
```json
{
  "placement": [
    {
      "id": "Cell1",
      "x": 0.0,
      "y": 0.0,
      "width": 120.0,
      "height": 80.0,
      "rotated": false
    }
  ],
  "metrics": {
    "total_hpwl": 1420.5,
    "total_area": 250000.0,
    "actual_width": 500.0,
    "actual_height": 500.0,
    "chip_utilization": 0.78,
    "final_cost": 1.15
  },
  "telemetry": {
    "runtime_ms": 142,
    "memory_mb": 4.8,
    "generations_completed": 100,
    "convergence_generation": 68
  },
  "fitness_history": [
    3250.0,
    2840.1,
    2100.4,
    1850.0
  ]
}
```
> **Примечание по совместимости телеметрии:**  
> В секции `"telemetry"` каноническим ключом по ТЗ является `"memory_mb"`. Модуль `io::JsonWriter` всегда генерирует `"memory_mb"`, а десериализатор `io::JsonParser` для обратной совместимости принимает оба ключа: `"memory_mb"` и `"peak_memory_mb"`.

