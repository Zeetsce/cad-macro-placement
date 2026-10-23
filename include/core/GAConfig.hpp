#pragma once

namespace core {

/**
 * @brief Метод селекции родителей.
 */
enum class SelectionMethod {
    TOURNAMENT, // Турнирная селекция (по умолчанию)
    ROULETTE    // Рулеточная селекция
};

/**
 * @brief Тип кроссовера перестановок sp
 */
enum class CrossoverType {
    ORDER_CROSSOVER,
    PARTIALLY_MAPPED_CROSSOVER
};

/**
 * @brief Конфигурация эволюционного поиска
 */
struct GAConfig {
    int population_size = 50;       // Размер популяции N_pop >= 4
    int generations = 100;          // Число поколений N_gen >= 1
    int tournament_size = 3;        // Размер турнирной группы 2 <= K <= N_pop
    int elit_count = 2;             // Количество элитных особей 0 <= elit < N_pop

    double crossover_prob = 0.8;    // Вероятность скрещивания [0.0, 1.0]
    double mutation_prob = 0.15;    // Вероятность мутации [0.0, 1.0]

    double hpwl_weight = 0.7;       // Вес HPWL в целевой функции (>= 0.0)
    double area_weight = 0.3;       // Вес площади в целевой функции (>= 0.0)

    SelectionMethod selection = SelectionMethod::TOURNAMENT;
    CrossoverType crossover = CrossoverType::ORDER_CROSSOVER;
};

} // namespace core
