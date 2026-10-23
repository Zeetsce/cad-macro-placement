#pragma once

#include "core/Individual.hpp"
#include <vector>

namespace core {

/**
 * @brief Буфер поколений
 */
struct PopulationState {
    int current_generation = 0;

    std::vector<Individual> individuals;     // Текущее поколение
    std::vector<Individual> next_generation; // Буфер для потомков

    Individual best_ever;                    // Абсолютно лучшая особь
    std::vector<double> best_fitness_history;// История лучшего фитнеса
};

} // namespace core
