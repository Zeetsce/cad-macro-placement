#pragma once

#include <cstdint>

namespace core {

/**
 * @brief Профилирование времени, памяти и сходимости
 */
struct ExecutionStats {
    int64_t elapsed_time_ms = 0;     // Время работы в миллисекундах (chrono)
    double peak_memory_mb = 0.0;     // Пиковая оперативная память в МБ (WinAPI GetProcessMemoryInfo)
    int total_generations = 0;       // Пройдено поколений
    int convergence_generation = -1; // Поколение, на котором наступила стабилизация
};

} // namespace core
