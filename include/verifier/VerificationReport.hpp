#pragma once

#include <string>
#include <vector>

namespace verifier {

struct OverlapViolation {
    int block_a = -1;
    int block_b = -1;
    double intersection_area = 0.0;
};

struct BoundaryViolation {
    int block_id = -1;
    double overflow_x = 0.0;
    double overflow_y = 0.0;
};

/**
 * @brief Отчет проверки топологии
 */
struct VerificationReport {
    bool is_valid = false;
    std::vector<OverlapViolation> overlaps;
    std::vector<BoundaryViolation> boundary_escapes;
    std::vector<int> grid_violations;
    std::vector<int> illegal_rotations;
    std::vector<int> missing_or_duplicate_blocks;
    double recalculated_hpwl = 0.0;
    bool hpwl_matches = false;

    std::string to_string() const;
};

} // namespace verifier
