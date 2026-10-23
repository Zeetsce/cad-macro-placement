#pragma once

#include <vector>
#include <cstdint>
#include <limits>

namespace core {

/**
 * @brief Генотип особи в представлении Sequence Pair.
 */
struct Individual {
    std::vector<int> gamma_plus;   // Первая перестановка SP
    std::vector<int> gamma_minus;  //Вторая перестановка SP

    /// Флаги поворота
    std::vector<uint8_t> is_rotated;

    double fitness = std::numeric_limits<double>::infinity(); // Минимизируемый фитнес
    double raw_hpwl = 0.0;   // Абсолютный HPWL (кэш для отчёта)
    double raw_area = 0.0;   // Абсолютная площадь (кэш для отчёта)
    bool is_feasible = true; // Флаг непревышения границ кристалла
};

} // namespace core
