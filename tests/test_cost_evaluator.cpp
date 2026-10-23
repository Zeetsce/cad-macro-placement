#include <cmath>
#include <cstdlib>
#include <iostream>

#include "core/EvaluationScratchpad.hpp"
#include "core/Individual.hpp"
#include "core/InputContext.hpp"
#include "core/StandardCostEvaluator.hpp"

static bool test_weighted_hpwl() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    // Block 0: weight 1.0, Block 1: weight 3.0
    context.blocks = {
        {0, 100.0, 50.0, 1.0, false},
        {1, 100.0, 50.0, 3.0, false}
    };
    // Net: connects {0, 1}. Net weight = (1.0 + 3.0)/2 = 2.0
    context.nets = {
        {0, {0, 1}}
    };
    context.ga_config.hpwl_weight = 1.0;
    context.ga_config.area_weight = 0.0;

    core::Individual ind;
    core::EvaluationScratchpad pad;
    pad.resize(2);

    // Block 0: (0, 0), center (50, 25)
    pad.x[0] = 0.0; pad.y[0] = 0.0; pad.w[0] = 100.0; pad.h[0] = 50.0;
    pad.center_x[0] = 50.0; pad.center_y[0] = 25.0;

    // Block 1: (100, 0), center (150, 25)
    pad.x[1] = 100.0; pad.y[1] = 0.0; pad.w[1] = 100.0; pad.h[1] = 50.0;
    pad.center_x[1] = 150.0; pad.center_y[1] = 25.0;

    core::StandardCostEvaluator evaluator;
    evaluator.evaluate(ind, context, pad);

    // Raw net HPWL = (150-50) + (25-25) = 100.0
    // Weighted HPWL = 2.0 * 100.0 = 200.0
    if (std::abs(ind.raw_hpwl - 200.0) > 1e-5) {
        std::cerr << "[FAIL] Weighted HPWL mismatch: expected 200.0, got " << ind.raw_hpwl << std::endl;
        return false;
    }

    if (!ind.is_feasible) {
        std::cerr << "[FAIL] Layout unexpectedly marked infeasible!" << std::endl;
        return false;
    }

    return true;
}

static bool test_degenerate_nets() {
    core::InputContext context;
    context.chip = {500.0, 500.0, 10.0, false};
    context.blocks = {
        {0, 50.0, 50.0, 1.0, false}
    };
    // Вырожденная цепь из 1 блока
    context.nets = {
        {0, {0}}
    };

    core::Individual ind;
    core::EvaluationScratchpad pad;
    pad.resize(1);
    pad.x[0] = 0.0; pad.y[0] = 0.0; pad.w[0] = 50.0; pad.h[0] = 50.0;
    pad.center_x[0] = 25.0; pad.center_y[0] = 25.0;

    core::StandardCostEvaluator evaluator;
    evaluator.evaluate(ind, context, pad);

    if (ind.raw_hpwl != 0.0) {
        std::cerr << "[FAIL] Degenerate net did not yield 0.0 HPWL: " << ind.raw_hpwl << std::endl;
        return false;
    }

    return true;
}

static bool test_death_penalty_barrier() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    context.blocks = {
        {0, 100.0, 100.0, 1.0, false}
    };
    context.ga_config.hpwl_weight = 0.5;
    context.ga_config.area_weight = 0.5;

    // Допустимое размещение
    core::Individual feasible_ind;
    core::EvaluationScratchpad feasible_pad;
    feasible_pad.resize(1);
    feasible_pad.x[0] = 0.0; feasible_pad.y[0] = 0.0; feasible_pad.w[0] = 100.0; feasible_pad.h[0] = 100.0;
    feasible_pad.center_x[0] = 50.0; feasible_pad.center_y[0] = 50.0;

    core::StandardCostEvaluator evaluator;
    const double feasible_cost = evaluator.evaluate(feasible_ind, context, feasible_pad);

    if (!feasible_ind.is_feasible || feasible_cost >= 10.0) {
        std::cerr << "[FAIL] Feasible solution cost unexpectedly high: " << feasible_cost << std::endl;
        return false;
    }

    // Недопустимое размещение (выход за границу чипа на 10 мкм: 950 + 100 = 1050 > 1000)
    core::Individual infeasible_ind;
    core::EvaluationScratchpad infeasible_pad;
    infeasible_pad.resize(1);
    infeasible_pad.x[0] = 950.0; infeasible_pad.y[0] = 0.0; infeasible_pad.w[0] = 100.0; infeasible_pad.h[0] = 100.0;
    infeasible_pad.center_x[0] = 1000.0; infeasible_pad.center_y[0] = 50.0;

    const double infeasible_cost = evaluator.evaluate(infeasible_ind, context, infeasible_pad);

    if (infeasible_ind.is_feasible) {
        std::cerr << "[FAIL] Infeasible solution marked as feasible!" << std::endl;
        return false;
    }

    // Проверка барьера: Cost >= 10 000
    if (infeasible_cost < core::StandardCostEvaluator::kBarrierPenalty) {
        std::cerr << "[FAIL] Death penalty barrier violation: cost " << infeasible_cost << " < 10000.0" << std::endl;
        return false;
    }

    // Допустимое решение строго доминирует над недопустимым
    if (feasible_cost >= infeasible_cost) {
        std::cerr << "[FAIL] Feasible solution does not dominate infeasible!" << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "[RUN] Running test_cost_evaluator..." << std::endl;

    if (!test_weighted_hpwl()) return EXIT_FAILURE;
    if (!test_degenerate_nets()) return EXIT_FAILURE;
    if (!test_death_penalty_barrier()) return EXIT_FAILURE;

    std::cout << "[PASS] All test_cost_evaluator scenarios passed successfully." << std::endl;
    return EXIT_SUCCESS;
}
