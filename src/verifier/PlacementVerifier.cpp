#include "verifier/PlacementVerifier.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace verifier {

VerificationReport PlacementVerifier::verify(
    const core::InputContext& context,
    const std::vector<core::PlacedBlock>& blocks,
    double claimed_hpwl) const {

    VerificationReport report;

    // Контроль полноты и биективности блоков
    check_completeness(context.blocks.size(), blocks, report);

    // Аудит взаимных наложений AABB (O(N^2))
    check_overlaps(blocks, report);

    // Проверка границ кристалла и сетки (O(N))
    check_boundaries_and_grid(context, blocks, report);

    // Проверка санкционированности поворотов (O(N))
    check_rotations(context, blocks, report);

    // Независимый перерасчет HPWL
    report.recalculated_hpwl = compute_independent_hpwl(context, blocks);
    report.hpwl_matches = (std::abs(report.recalculated_hpwl - claimed_hpwl) < 1e-5);

    // Итоговый вердикт
    report.is_valid = report.overlaps.empty() &&
                      report.boundary_escapes.empty() &&
                      report.grid_violations.empty() &&
                      report.illegal_rotations.empty() &&
                      report.missing_or_duplicate_blocks.empty() &&
                      report.hpwl_matches;

    return report;
}

bool PlacementVerifier::is_geometrically_valid(
    const core::InputContext& context,
    const std::vector<core::PlacedBlock>& blocks) const {

    // Проверка наложений
    const size_t n = blocks.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const double x1 = std::max(blocks[i].x, blocks[j].x);
            const double x2 = std::min(blocks[i].x + blocks[i].width, blocks[j].x + blocks[j].width);
            const double y1 = std::max(blocks[i].y, blocks[j].y);
            const double y2 = std::min(blocks[i].y + blocks[i].height, blocks[j].y + blocks[j].height);

            if ((x2 - x1) > kEpsilon && (y2 - y1) > kEpsilon) {
                return false;
            }
        }
    }

    // Проверка выхода за границы
    const double chip_w = context.chip.width;
    const double chip_h = context.chip.height;
    for (const auto& b : blocks) {
        if (b.x < -kEpsilon || b.y < -kEpsilon ||
            (b.x + b.width) > chip_w + kEpsilon ||
            (b.y + b.height) > chip_h + kEpsilon) {
            return false;
        }
    }

    return true;
}

void PlacementVerifier::check_overlaps(
    const std::vector<core::PlacedBlock>& blocks,
    VerificationReport& report) const {

    const size_t n = blocks.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const double x1 = std::max(blocks[i].x, blocks[j].x);
            const double x2 = std::min(blocks[i].x + blocks[i].width, blocks[j].x + blocks[j].width);
            const double y1 = std::max(blocks[i].y, blocks[j].y);
            const double y2 = std::min(blocks[i].y + blocks[i].height, blocks[j].y + blocks[j].height);

            if ((x2 - x1) > kEpsilon && (y2 - y1) > kEpsilon) {
                const double overlap_area = (x2 - x1) * (y2 - y1);
                report.overlaps.push_back({blocks[i].block_id, blocks[j].block_id, overlap_area});
            }
        }
    }
}

void PlacementVerifier::check_boundaries_and_grid(
    const core::InputContext& context,
    const std::vector<core::PlacedBlock>& blocks,
    VerificationReport& report) const {

    const double chip_w = context.chip.width;
    const double chip_h = context.chip.height;
    const double grid = context.chip.grid_size;

    for (const auto& b : blocks) {
        // Проверка границ
        const double overflow_x = std::max(0.0, (b.x + b.width) - chip_w) + std::max(0.0, -b.x);
        const double overflow_y = std::max(0.0, (b.y + b.height) - chip_h) + std::max(0.0, -b.y);

        if (overflow_x > kEpsilon || overflow_y > kEpsilon) {
            report.boundary_escapes.push_back({b.block_id, overflow_x, overflow_y});
        }

        // Проверка кратности сетке
        if (grid > kEpsilon) {
            double rem_x = std::fmod(b.x, grid);
            if (rem_x < 0.0) {
                rem_x += grid;
            }
            const bool bad_x = (rem_x > kEpsilon && rem_x < (grid - kEpsilon));

            double rem_y = std::fmod(b.y, grid);
            if (rem_y < 0.0) {
                rem_y += grid;
            }
            const bool bad_y = (rem_y > kEpsilon && rem_y < (grid - kEpsilon));

            if (bad_x || bad_y) {
                report.grid_violations.push_back(b.block_id);
            }
        }
    }
}

void PlacementVerifier::check_rotations(
    const core::InputContext& context,
    const std::vector<core::PlacedBlock>& blocks,
    VerificationReport& report) const {

    for (const auto& b : blocks) {
        if (b.block_id < 0 || static_cast<size_t>(b.block_id) >= context.blocks.size()) {
            continue; // Некорректный ID обрабатывается в check_completeness
        }

        const auto& orig = context.blocks[static_cast<size_t>(b.block_id)];
        const bool rotated_by_flag = b.is_rotated;
        const bool rotated_by_dim = (std::abs(b.width - orig.height) < kEpsilon &&
                                     std::abs(b.height - orig.width) < kEpsilon &&
                                     std::abs(orig.width - orig.height) > kEpsilon);

        if (rotated_by_flag || rotated_by_dim) {
            if (!context.chip.rotation_allowed || !orig.rotation_allowed) {
                report.illegal_rotations.push_back(b.block_id);
            }
        }
    }
}

void PlacementVerifier::check_completeness(
    size_t expected_count,
    const std::vector<core::PlacedBlock>& blocks,
    VerificationReport& report) const {

    if (blocks.size() != expected_count) {
        report.missing_or_duplicate_blocks.push_back(-1); // Маркер несовпадения размера
    }

    std::vector<int> counts(expected_count, 0);
    for (const auto& b : blocks) {
        if (b.block_id >= 0 && static_cast<size_t>(b.block_id) < expected_count) {
            counts[static_cast<size_t>(b.block_id)]++;
        } else {
            report.missing_or_duplicate_blocks.push_back(b.block_id);
        }
    }

    for (size_t id = 0; id < expected_count; ++id) {
        if (counts[id] != 1) {
            report.missing_or_duplicate_blocks.push_back(static_cast<int>(id));
        }
    }
}

double PlacementVerifier::compute_independent_hpwl(
    const core::InputContext& context,
    const std::vector<core::PlacedBlock>& blocks) const {

    if (context.nets.empty() || blocks.empty()) {
        return 0.0;
    }

    // Построение карты центров по block_id
    std::vector<double> center_x(context.blocks.size(), 0.0);
    std::vector<double> center_y(context.blocks.size(), 0.0);

    for (const auto& b : blocks) {
        if (b.block_id >= 0 && static_cast<size_t>(b.block_id) < context.blocks.size()) {
            center_x[static_cast<size_t>(b.block_id)] = b.x + (b.width / 2.0);
            center_y[static_cast<size_t>(b.block_id)] = b.y + (b.height / 2.0);
        }
    }

    double total_hpwl = 0.0;

    for (const auto& net : context.nets) {
        if (net.connected_block_ids.size() <= 1) {
            continue; // Вырожденная цепь
        }

        double min_x = std::numeric_limits<double>::infinity();
        double max_x = -std::numeric_limits<double>::infinity();
        double min_y = std::numeric_limits<double>::infinity();
        double max_y = -std::numeric_limits<double>::infinity();

        double weight_sum = 0.0;

        for (int bid : net.connected_block_ids) {
            if (bid >= 0 && static_cast<size_t>(bid) < context.blocks.size()) {
                const double cx = center_x[static_cast<size_t>(bid)];
                const double cy = center_y[static_cast<size_t>(bid)];

                min_x = std::min(min_x, cx);
                max_x = std::max(max_x, cx);
                min_y = std::min(min_y, cy);
                max_y = std::max(max_y, cy);

                weight_sum += context.blocks[static_cast<size_t>(bid)].weight;
            }
        }

        const double net_hpwl = (max_x - min_x) + (max_y - min_y);
        const double net_weight = weight_sum / static_cast<double>(net.connected_block_ids.size());

        total_hpwl += net_weight * net_hpwl;
    }

    return total_hpwl;
}

} // namespace verifier
