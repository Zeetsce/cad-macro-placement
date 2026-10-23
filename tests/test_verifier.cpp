#include <cmath>
#include <cstdlib>
#include <iostream>

#include "core/InputContext.hpp"
#include "core/PlacedBlock.hpp"
#include "verifier/PlacementVerifier.hpp"

static bool test_valid_layout() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, true};
    context.blocks = {
        {0, 100.0, 100.0, 1.0, true},
        {1, 100.0, 100.0, 1.0, true}
    };
    context.nets = {
        {0, {0, 1}}
    };

    // Два блока касаются стороной: A[0..100], B[100..200]
    std::vector<core::PlacedBlock> blocks = {
        {0, 0.0, 0.0, 100.0, 100.0, 50.0, 50.0, false},
        {1, 100.0, 0.0, 100.0, 100.0, 150.0, 50.0, false}
    };

    // HPWL: centers A(50, 50), B(150, 50) -> dx = 100, dy = 0 -> HPWL = 100.0
    const double expected_hpwl = 100.0;

    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, expected_hpwl);

    if (!report.is_valid || !report.hpwl_matches || !report.overlaps.empty() ||
        !report.boundary_escapes.empty() || !report.grid_violations.empty() ||
        !report.illegal_rotations.empty() || !report.missing_or_duplicate_blocks.empty()) {
        std::cerr << "[FAIL] Valid layout was reported as invalid:\n" << report.to_string() << std::endl;
        return false;
    }

    if (!verifier.is_geometrically_valid(context, blocks)) {
        std::cerr << "[FAIL] is_geometrically_valid returned false for valid layout!" << std::endl;
        return false;
    }

    return true;
}

static bool test_catches_exact_overlap() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    context.blocks = {
        {0, 50.0, 50.0, 1.0, false},
        {1, 50.0, 50.0, 1.0, false}
    };

    // Блоки пересекаются: A[0..50, 0..50] и B[40..90, 40..90] -> пересечение [40..50, 40..50], area = 100
    std::vector<core::PlacedBlock> blocks = {
        {0, 0.0, 0.0, 50.0, 50.0, 25.0, 25.0, false},
        {1, 40.0, 40.0, 50.0, 50.0, 65.0, 65.0, false}
    };

    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, 0.0);

    if (report.is_valid || report.overlaps.size() != 1) {
        std::cerr << "[FAIL] Overlap not caught as expected!" << std::endl;
        return false;
    }

    if (std::abs(report.overlaps[0].intersection_area - 100.0) > 1e-5) {
        std::cerr << "[FAIL] Overlap area calculation mismatch: " << report.overlaps[0].intersection_area << std::endl;
        return false;
    }

    if (verifier.is_geometrically_valid(context, blocks)) {
        std::cerr << "[FAIL] is_geometrically_valid returned true for overlapping layout!" << std::endl;
        return false;
    }

    return true;
}

static bool test_catches_boundary_escape() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    context.blocks = {
        {0, 50.0, 50.0, 1.0, false}
    };

    // Блок выходит за правую границу: x=980 + w=50 = 1030 > 1000 (overflow_x = 30)
    std::vector<core::PlacedBlock> blocks = {
        {0, 980.0, 0.0, 50.0, 50.0, 1005.0, 25.0, false}
    };

    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, 0.0);

    if (report.is_valid || report.boundary_escapes.size() != 1) {
        std::cerr << "[FAIL] Boundary escape not caught!" << std::endl;
        return false;
    }

    if (std::abs(report.boundary_escapes[0].overflow_x - 30.0) > 1e-5) {
        std::cerr << "[FAIL] Boundary overflow_x mismatch: " << report.boundary_escapes[0].overflow_x << std::endl;
        return false;
    }

    return true;
}

static bool test_detects_illegal_rotation() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false}; // Поворот на кристалле запрещен
    context.blocks = {
        {0, 40.0, 20.0, 1.0, false}
    };

    // Размещен с поворотом 20x40
    std::vector<core::PlacedBlock> blocks = {
        {0, 0.0, 0.0, 20.0, 40.0, 10.0, 20.0, true}
    };

    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, 0.0);

    if (report.is_valid || report.illegal_rotations.size() != 1 || report.illegal_rotations[0] != 0) {
        std::cerr << "[FAIL] Illegal rotation not detected!" << std::endl;
        return false;
    }

    return true;
}

static bool test_detects_grid_violation() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    context.blocks = {
        {0, 40.0, 20.0, 1.0, false}
    };

    // x = 15.0 не кратно шагу сетки 10.0
    std::vector<core::PlacedBlock> blocks = {
        {0, 15.0, 0.0, 40.0, 20.0, 35.0, 10.0, false}
    };

    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, 0.0);

    if (report.is_valid || report.grid_violations.size() != 1 || report.grid_violations[0] != 0) {
        std::cerr << "[FAIL] Grid violation not detected!" << std::endl;
        return false;
    }

    return true;
}

static bool test_catches_hpwl_mismatch() {
    core::InputContext context;
    context.chip = {1000.0, 1000.0, 10.0, false};
    context.blocks = {
        {0, 100.0, 100.0, 1.0, false},
        {1, 100.0, 100.0, 1.0, false}
    };
    context.nets = {
        {0, {0, 1}}
    };

    std::vector<core::PlacedBlock> blocks = {
        {0, 0.0, 0.0, 100.0, 100.0, 50.0, 50.0, false},
        {1, 100.0, 0.0, 100.0, 100.0, 150.0, 50.0, false}
    };

    // Фактический HPWL = 100.0, передаем 150.0
    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, blocks, 150.0);

    if (report.is_valid || report.hpwl_matches) {
        std::cerr << "[FAIL] HPWL mismatch not detected!" << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "[RUN] Running test_verifier..." << std::endl;

    if (!test_valid_layout()) return EXIT_FAILURE;
    if (!test_catches_exact_overlap()) return EXIT_FAILURE;
    if (!test_catches_boundary_escape()) return EXIT_FAILURE;
    if (!test_detects_illegal_rotation()) return EXIT_FAILURE;
    if (!test_detects_grid_violation()) return EXIT_FAILURE;
    if (!test_catches_hpwl_mismatch()) return EXIT_FAILURE;

    std::cout << "[PASS] All test_verifier scenarios passed successfully." << std::endl;
    return EXIT_SUCCESS;
}
