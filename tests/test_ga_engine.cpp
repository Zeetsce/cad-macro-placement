#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "core/DagPlacementDecoder.hpp"
#include "core/GARunner.hpp"
#include "core/StandardCostEvaluator.hpp"
#include "io/JsonIO.hpp"
#include "verifier/PlacementVerifier.hpp"

static bool is_valid_permutation(const std::vector<int>& perm) {
    const size_t n = perm.size();
    std::vector<int> count(n, 0);
    for (int val : perm) {
        if (val < 0 || static_cast<size_t>(val) >= n) {
            return false;
        }
        count[static_cast<size_t>(val)]++;
    }
    for (size_t i = 0; i < n; ++i) {
        if (count[i] != 1) return false;
    }
    return true;
}

static bool test_crossover_bijectivity() {
    core::DagPlacementDecoder decoder;
    core::StandardCostEvaluator evaluator;
    core::GARunner runner(decoder, evaluator);

    std::mt19937 rng(42);

    for (size_t n : {10, 50, 100}) {
        std::vector<int> p1(n), p2(n), child(n);
        std::iota(p1.begin(), p1.end(), 0);
        std::iota(p2.begin(), p2.end(), 0);

        for (int trial = 0; trial < 100; ++trial) {
            std::shuffle(p1.begin(), p1.end(), rng);
            std::shuffle(p2.begin(), p2.end(), rng);

            // Тест Order Crossover (OX)
            runner.order_crossover(p1, p2, child, rng);
            if (!is_valid_permutation(child)) {
                std::cerr << "[FAIL] Order crossover produced non-bijective permutation at N=" << n << std::endl;
                return false;
            }

            // Тест Partially Mapped Crossover (PMX)
            runner.pmx_crossover(p1, p2, child, rng);
            if (!is_valid_permutation(child)) {
                std::cerr << "[FAIL] PMX crossover produced non-bijective permutation at N=" << n << std::endl;
                return false;
            }
        }
    }

    return true;
}

static bool test_rotation_lock() {
    core::DagPlacementDecoder decoder;
    core::StandardCostEvaluator evaluator;
    core::GARunner runner(decoder, evaluator);

    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, true};
    context.blocks = {
        {0, 50.0, 50.0, 1.0, false}, // Запрещен поворот блока
        {1, 50.0, 50.0, 1.0, true}   // Разрешен поворот блока
    };

    std::mt19937 rng(123);
    std::vector<uint8_t> is_rot = {0, 0};

    for (int trial = 0; trial < 200; ++trial) {
        runner.mutate_rotate(is_rot, context, rng);
        if (is_rot[0] != 0) {
            std::cerr << "[FAIL] Block 0 with rotation_allowed=false was mutated to rotated!" << std::endl;
            return false;
        }
    }

    // Тест с глобальным запретом на чипе
    context.chip.rotation_allowed = false;
    is_rot = {0, 0};
    for (int trial = 0; trial < 200; ++trial) {
        runner.mutate_rotate(is_rot, context, rng);
        if (is_rot[0] != 0 || is_rot[1] != 0) {
            std::cerr << "[FAIL] Rotation occurred when chip.rotation_allowed=false!" << std::endl;
            return false;
        }
    }

    return true;
}

static bool test_convergence_calculation() {
    core::DagPlacementDecoder decoder;
    core::StandardCostEvaluator evaluator;
    core::GARunner runner(decoder, evaluator);

    // Моделируем историю: улучшение до поколения 20, далее стабилизация на 10.0
    const int total_gen = 100;
    std::vector<double> history(total_gen);
    for (int i = 0; i < 20; ++i) {
        history[i] = 100.0 - static_cast<double>(i) * 4.5;
    }
    for (int i = 20; i < total_gen; ++i) {
        history[i] = 10.0;
    }

    const int conv_gen = runner.compute_convergence_generation(history, total_gen);
    if (conv_gen != 20) {
        std::cerr << "[FAIL] Convergence generation mismatch: expected 20, got " << conv_gen << std::endl;
        return false;
    }

    return true;
}

static bool test_full_ga_run_and_elitism() {
    core::DagPlacementDecoder decoder;
    core::StandardCostEvaluator evaluator;
    core::GARunner runner(decoder, evaluator);

    // Загрузка эталонного файла задачи
    std::string sample_path = "data/sample_team1.json";
    std::ifstream check(sample_path);
    if (!check.good()) {
        sample_path = "../data/sample_team1.json";
    }
    auto context = io::JsonIO::parse_input_file(sample_path);
    context.ga_config.population_size = 20;
    context.ga_config.generations = 30;

    const auto result = runner.run(context, 777);

    // Проверка монотонности истории лучшего фитнеса (элитизм)
    if (result.fitness_history.empty()) {
        std::cerr << "[FAIL] Fitness history is empty!" << std::endl;
        return false;
    }

    for (size_t i = 1; i < result.fitness_history.size(); ++i) {
        if (result.fitness_history[i] > result.fitness_history[i - 1] + 1e-9) {
            std::cerr << "[FAIL] Monotonicity violation in elitism: gen " << i
                      << " (" << result.fitness_history[i] << ") > gen " << (i - 1)
                      << " (" << result.fitness_history[i - 1] << ")" << std::endl;
            return false;
        }
    }

    // Аудит полученного размещения независимым верификатором
    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, result.blocks, result.metrics.total_hpwl);

    if (!report.is_valid || !report.overlaps.empty() || !report.boundary_escapes.empty() ||
        !report.grid_violations.empty() || !report.illegal_rotations.empty() ||
        !report.missing_or_duplicate_blocks.empty() || !report.hpwl_matches) {
        std::cerr << "[FAIL] GA generated invalid placement according to verifier:\n"
                  << report.to_string() << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "[RUN] Running test_ga_engine..." << std::endl;

    if (!test_crossover_bijectivity()) return EXIT_FAILURE;
    if (!test_rotation_lock()) return EXIT_FAILURE;
    if (!test_convergence_calculation()) return EXIT_FAILURE;
    if (!test_full_ga_run_and_elitism()) return EXIT_FAILURE;

    std::cout << "[PASS] All test_ga_engine scenarios passed successfully." << std::endl;
    return EXIT_SUCCESS;
}
