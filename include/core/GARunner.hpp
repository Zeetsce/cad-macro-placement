#pragma once

#include "core/ICostEvaluator.hpp"
#include "core/IGAEngine.hpp"
#include "core/IPlacementDecoder.hpp"
#include "core/Individual.hpp"
#include "core/PopulationState.hpp"

#include <random>
#include <vector>

namespace core {

/**
 * @brief Движок генетического алгоритма Sequence Pair
 */
class GARunner : public IGAEngine {
public:
    GARunner(const IPlacementDecoder& decoder, const ICostEvaluator& evaluator);
    ~GARunner() override = default;

    /**
     * @brief Запуск генетического алгоритма
     * @param context Данные схемы
     * @param seed Зерно генератора случайных чисел
     * @return Результат размещения и статистика
     */
    PlacementResult run(const InputContext& context, uint32_t seed = 42) override;

    int tournament_select(
        const std::vector<Individual>& pop,
        int tournament_size,
        std::mt19937& rng) const;

    void order_crossover(
        const std::vector<int>& p1,
        const std::vector<int>& p2,
        std::vector<int>& child,
        std::mt19937& rng) const;

    void pmx_crossover(
        const std::vector<int>& p1,
        const std::vector<int>& p2,
        std::vector<int>& child,
        std::mt19937& rng) const;

    void mutate_swap(
        std::vector<int>& perm,
        std::mt19937& rng) const;

    void mutate_rotate(
        std::vector<uint8_t>& is_rot,
        const InputContext& context,
        std::mt19937& rng) const;

    int compute_convergence_generation(
        const std::vector<double>& history,
        int total_generations) const;

private:
    const IPlacementDecoder& decoder_;
    const ICostEvaluator& evaluator_;

    void initialize_population(
        PopulationState& state,
        const InputContext& context,
        std::mt19937& rng) const;

    void evaluate_population_parallel(
        std::vector<Individual>& pop,
        const InputContext& context) const;
};

} // namespace core
