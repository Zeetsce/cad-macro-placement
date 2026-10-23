#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "core/BlockData.hpp"
#include "core/ChipSpec.hpp"
#include "core/EvaluationScratchpad.hpp"
#include "core/ExecutionStats.hpp"
#include "core/GAConfig.hpp"
#include "core/Individual.hpp"
#include "core/InputContext.hpp"
#include "core/NetData.hpp"
#include "core/PlacedBlock.hpp"
#include "core/PlacementResult.hpp"
#include "core/PopulationState.hpp"
#include "core/QualityMetrics.hpp"
#include "verifier/VerificationReport.hpp"

/**
 * @brief Валидация типов данных и значений по умолчанию структур ядра и верификатора.
 *
 * @return int EXIT_SUCCESS при успешной проверке, EXIT_FAILURE при обнаружении несоответствий.
 *
 * @pre Заголовочные файлы включают все контракты ядра (include/core/) и верификатора (include/verifier/).
 * @post Проверены дефолтные значения, корректность типов, буферы Scratchpad и агрегаты данных.
 *
 * @note Time complexity: O(1)
 * @note Space complexity: O(1)
 */
int main() {
    std::cout << "[RUN] Running test_data_formats..." << std::endl;

    // Входной контракт: ChipSpec
    core::ChipSpec chip;
    if (chip.width != 0.0 || chip.height != 0.0 || chip.grid_size != 1.0 || chip.rotation_allowed) {
        std::cerr << "[FAIL] ChipSpec default values mismatch!" << std::endl;
        return EXIT_FAILURE;
    }

    // Входной контракт: BlockData
    core::BlockData block;
    block.id = 0;
    block.width = 100.0;
    block.height = 50.0;
    block.weight = 1.0;
    block.rotation_allowed = true;
    if (block.id != 0 || block.width != 100.0 || block.height != 50.0 || block.weight != 1.0 || !block.rotation_allowed) {
        std::cerr << "[FAIL] BlockData field initialization error!" << std::endl;
        return EXIT_FAILURE;
    }

    // Входной контракт: NetData
    core::NetData net;
    net.id = 1;
    net.connected_block_ids = {0, 1, 2};
    if (net.id != 1 || net.connected_block_ids.size() != 3) {
        std::cerr << "[FAIL] NetData initialization error!" << std::endl;
        return EXIT_FAILURE;
    }

    // Входной контракт: GAConfig
    core::GAConfig ga;
    if (ga.population_size != 50 || ga.generations != 100 || ga.tournament_size != 3 ||
        ga.elit_count != 2 || std::abs(ga.crossover_prob - 0.8) > 1e-9 ||
        std::abs(ga.mutation_prob - 0.15) > 1e-9 || std::abs(ga.hpwl_weight - 0.7) > 1e-9 ||
        std::abs(ga.area_weight - 0.3) > 1e-9 ||
        ga.selection != core::SelectionMethod::TOURNAMENT ||
        ga.crossover != core::CrossoverType::ORDER_CROSSOVER) {
        std::cerr << "[FAIL] GAConfig default parameters mismatch!" << std::endl;
        return EXIT_FAILURE;
    }

    // Консолидированный InputContext
    core::InputContext context;
    context.chip = chip;
    context.blocks.push_back(block);
    context.nets.push_back(net);
    context.ga_config = ga;
    context.index_to_block_name.push_back("Cell1");
    context.block_name_to_index["Cell1"] = 0;
    context.index_to_net_name.push_back("Net1");
    if (context.blocks.size() != 1 || context.nets.size() != 1 || context.block_name_to_index.at("Cell1") != 0) {
        std::cerr << "[FAIL] InputContext verification error!" << std::endl;
        return EXIT_FAILURE;
    }

    // Runtime: Individual
    core::Individual ind;
    ind.gamma_plus = {0, 1};
    ind.gamma_minus = {1, 0};
    ind.is_rotated = {0, 1};
    if (!std::isinf(ind.fitness) || !ind.is_feasible || ind.is_rotated.size() != 2) {
        std::cerr << "[FAIL] Individual default state error!" << std::endl;
        return EXIT_FAILURE;
    }

    //  Runtime: EvaluationScratchpad
    core::EvaluationScratchpad pad;
    const size_t kBlockCount = 100;
    pad.resize(kBlockCount);
    if (pad.x.size() != kBlockCount || pad.y.size() != kBlockCount || pad.w.size() != kBlockCount ||
        pad.h.size() != kBlockCount || pad.center_x.size() != kBlockCount ||
        pad.center_y.size() != kBlockCount || pad.position_in_minus.size() != kBlockCount ||
        pad.length_buffer.size() != kBlockCount) {
        std::cerr << "[FAIL] EvaluationScratchpad buffer size mismatch after resize!" << std::endl;
        return EXIT_FAILURE;
    }

    //  Runtime: PopulationState
    core::PopulationState pop;
    pop.current_generation = 0;
    pop.individuals.push_back(ind);
    pop.best_fitness_history.push_back(100.0);
    if (pop.individuals.size() != 1 || pop.best_fitness_history.empty()) {
        std::cerr << "[FAIL] PopulationState verification error!" << std::endl;
        return EXIT_FAILURE;
    }

    // Выходной контракт: PlacedBlock, QualityMetrics, ExecutionStats
    core::PlacedBlock placed;
    placed.block_id = 0;
    placed.x = 10.0;
    placed.y = 20.0;
    placed.width = 100.0;
    placed.height = 50.0;
    placed.center_x = 60.0;
    placed.center_y = 45.0;
    placed.is_rotated = false;

    core::QualityMetrics metrics;
    metrics.total_hpwl = 120.0;
    metrics.total_area = 5000.0;
    metrics.actual_width = 100.0;
    metrics.actual_height = 50.0;
    metrics.chip_utilization = 0.8;
    metrics.final_cost = 1.05;

    core::ExecutionStats stats;
    stats.elapsed_time_ms = 42;
    stats.peak_memory_mb = 3.5;
    stats.total_generations = 100;
    stats.convergence_generation = 45;

    // Верификатор: VerificationReport и PlacementResult
    verifier::VerificationReport report;
    report.is_valid = true;
    report.hpwl_matches = true;
    report.recalculated_hpwl = 120.0;

    core::PlacementResult result;
    result.blocks.push_back(placed);
    result.metrics = metrics;
    result.stats = stats;
    result.fitness_history = pop.best_fitness_history;
    result.verification = report;

    if (result.blocks.empty() || !result.verification.is_valid ||
        result.stats.elapsed_time_ms != 42 || result.metrics.total_hpwl != 120.0) {
        std::cerr << "[FAIL] PlacementResult verification error!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[PASS] test_data_formats passed successfully. All data contracts are verified." << std::endl;
    return EXIT_SUCCESS;
}
