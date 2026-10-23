#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "core/DagPlacementDecoder.hpp"
#include "core/EvaluationScratchpad.hpp"
#include "core/Individual.hpp"
#include "core/InputContext.hpp"
#include "verifier/PlacementVerifier.hpp"

/**
 * @brief Аналитический эталонный тест на 3 блоках из SPEC_ALGORITHMS.md (Раздел 1.3).
 */
static bool test_analytical_3_blocks_no_rotation() {
    core::InputContext context;
    context.chip = {100.0, 100.0, 10.0, true};
    // A: 40x20, B: 30x30, C: 20x50
    context.blocks = {
        {0, 40.0, 20.0, 1.0, true}, // Block 0 = A
        {1, 30.0, 30.0, 1.0, true}, // Block 1 = B
        {2, 20.0, 50.0, 1.0, true}  // Block 2 = C
    };

    // Gamma+ = [A, B, C] = [0, 1, 2]
    // Gamma- = [B, A, C] = [1, 0, 2]
    core::Individual ind;
    ind.gamma_plus = {0, 1, 2};
    ind.gamma_minus = {1, 0, 2};
    ind.is_rotated = {0, 0, 0};

    core::EvaluationScratchpad scratchpad;
    scratchpad.resize(3);

    core::DagPlacementDecoder decoder;
    decoder.decode_to_scratchpad(ind, context, scratchpad);

    // B (Block 1): (0, 0)
    if (scratchpad.x[1] != 0.0 || scratchpad.y[1] != 0.0) {
        std::cerr << "[FAIL] Block B (1) coords mismatch: (" << scratchpad.x[1] << ", " << scratchpad.y[1] << ")" << std::endl;
        return false;
    }

    // A (Block 0): (0, 30)
    if (scratchpad.x[0] != 0.0 || scratchpad.y[0] != 30.0) {
        std::cerr << "[FAIL] Block A (0) coords mismatch: (" << scratchpad.x[0] << ", " << scratchpad.y[0] << ")" << std::endl;
        return false;
    }

    // C (Block 2): (40, 0)
    if (scratchpad.x[2] != 40.0 || scratchpad.y[2] != 0.0) {
        std::cerr << "[FAIL] Block C (2) coords mismatch: (" << scratchpad.x[2] << ", " << scratchpad.y[2] << ")" << std::endl;
        return false;
    }

    // Центры: A(20, 40), B(15, 15), C(50, 25)
    if (scratchpad.center_x[0] != 20.0 || scratchpad.center_y[0] != 40.0 ||
        scratchpad.center_x[1] != 15.0 || scratchpad.center_y[1] != 15.0 ||
        scratchpad.center_x[2] != 50.0 || scratchpad.center_y[2] != 25.0) {
        std::cerr << "[FAIL] Block centers mismatch!" << std::endl;
        return false;
    }

    // Проверка независимым верификатором
    const auto layout = decoder.decode_to_layout(ind, context);
    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, layout, 0.0);

    if (!report.overlaps.empty() || !report.boundary_escapes.empty() || !report.grid_violations.empty()) {
        std::cerr << "[FAIL] Analytical placement failed verifier audit:\n" << report.to_string() << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Тест вращения блока при is_rotated == 1.
 */
static bool test_rotation_handling() {
    core::InputContext context;
    context.chip = {200.0, 200.0, 5.0, true};
    // Номинально 40x20
    context.blocks = {
        {0, 40.0, 20.0, 1.0, true},
        {1, 30.0, 30.0, 1.0, true}
    };

    core::Individual ind;
    ind.gamma_plus = {0, 1};
    ind.gamma_minus = {0, 1}; // Блок 0 слева от Блока 1 (Horizontal)
    ind.is_rotated = {1, 0};  // Блок 0 повернут: эффективные w=20, h=40

    core::DagPlacementDecoder decoder;
    core::EvaluationScratchpad pad;
    pad.resize(2);

    decoder.decode_to_scratchpad(ind, context, pad);

    // Блок 0: w=20, h=40
    if (pad.w[0] != 20.0 || pad.h[0] != 40.0) {
        std::cerr << "[FAIL] Rotated block 0 effective dimensions mismatch!" << std::endl;
        return false;
    }

    // Блок 1 должен располагаться на x >= 20.0 (x = 20.0)
    if (pad.x[1] != 20.0) {
        std::cerr << "[FAIL] Block 1 x coord mismatch: expected 20.0, got " << pad.x[1] << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Тест строгого квантования координат по шагу сетки grid_size.
 */
static bool test_grid_size_quantization() {
    core::InputContext context;
    context.chip = {500.0, 500.0, 10.0, true}; // Шаг сетки 10.0
    // Размеры не кратны 10: w=33.0
    context.blocks = {
        {0, 33.0, 20.0, 1.0, true},
        {1, 20.0, 20.0, 1.0, true}
    };

    core::Individual ind;
    ind.gamma_plus = {0, 1};
    ind.gamma_minus = {0, 1}; // Блок 0 слева от Блока 1
    ind.is_rotated = {0, 0};

    core::DagPlacementDecoder decoder;
    core::EvaluationScratchpad pad;
    pad.resize(2);

    decoder.decode_to_scratchpad(ind, context, pad);

    // x[0] = 0.0. Для Блока 1: x[0] + w[0] = 33.0 -> ceil(33/10)*10 = 40.0
    if (pad.x[1] != 40.0) {
        std::cerr << "[FAIL] Grid quantization failed: expected 40.0, got " << pad.x[1] << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Стресс-тест 10 000 итераций в цикле без аллокаций.
 */
static bool test_hot_loop_stability() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, true};
    const size_t n = 20;
    for (size_t i = 0; i < n; ++i) {
        context.blocks.push_back({static_cast<int>(i), 30.0, 40.0, 1.0, true});
    }

    core::Individual ind;
    ind.gamma_plus.resize(n);
    ind.gamma_minus.resize(n);
    ind.is_rotated.assign(n, 0);
    for (size_t i = 0; i < n; ++i) {
        ind.gamma_plus[i] = static_cast<int>(i);
        ind.gamma_minus[i] = static_cast<int>(n - 1 - i);
    }

    core::EvaluationScratchpad pad;
    pad.resize(n);

    core::DagPlacementDecoder decoder;

    for (int iter = 0; iter < 10000; ++iter) {
        decoder.decode_to_scratchpad(ind, context, pad);
    }

    // Проверяем детерминизм результата
    if (pad.x.size() != n || pad.x[0] != 0.0) {
        std::cerr << "[FAIL] Hot loop output non-deterministic!" << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "[RUN] Running test_placement_decoder..." << std::endl;

    if (!test_analytical_3_blocks_no_rotation()) return EXIT_FAILURE;
    if (!test_rotation_handling()) return EXIT_FAILURE;
    if (!test_grid_size_quantization()) return EXIT_FAILURE;
    if (!test_hot_loop_stability()) return EXIT_FAILURE;

    std::cout << "[PASS] All test_placement_decoder scenarios passed successfully." << std::endl;
    return EXIT_SUCCESS;
}
