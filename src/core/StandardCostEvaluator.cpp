#include "core/StandardCostEvaluator.hpp"

#include <algorithm>
#include <limits>

namespace core {

double StandardCostEvaluator::evaluate(
    Individual& individual,
    const InputContext& context,
    const EvaluationScratchpad& scratchpad) const noexcept {

    const size_t n = context.blocks.size();
    if (n == 0) {
        individual.fitness = 0.0;
        individual.raw_hpwl = 0.0;
        individual.raw_area = 0.0;
        individual.is_feasible = true;
        return 0.0;
    }

    // Вычисление фактических габаритов охватывающего прямоугольника
    double actual_w = 0.0;
    double actual_h = 0.0;

    for (size_t i = 0; i < n; ++i) {
        const double right = scratchpad.x[i] + scratchpad.w[i];
        const double top = scratchpad.y[i] + scratchpad.h[i];
        if (right > actual_w) {
            actual_w = right;
        }
        if (top > actual_h) {
            actual_h = top;
        }
    }

    const double actual_area = actual_w * actual_h;

    // Аудит допустимости и расчет штрафа
    const double chip_w = context.chip.width;
    const double chip_h = context.chip.height;

    const double delta_w = std::max(0.0, actual_w - chip_w);
    const double delta_h = std::max(0.0, actual_h - chip_h);
    const bool is_feasible = (delta_w <= kEpsilon && delta_h <= kEpsilon);

    double penalty = 0.0;
    if (!is_feasible) {
        const double norm_dw = (chip_w > 0.0) ? (delta_w / chip_w) : delta_w;
        const double norm_dh = (chip_h > 0.0) ? (delta_h / chip_h) : delta_h;
        const double sum_norm = norm_dw + norm_dh;
        penalty = kBarrierPenalty + kPenaltyCoeff * (sum_norm * sum_norm);
    }

    // Вычисление взвешенного HPWL
    double total_hpwl = 0.0;
    for (const auto& net : context.nets) {
        if (net.connected_block_ids.size() <= 1) {
            continue;
        }

        double min_x = std::numeric_limits<double>::infinity();
        double max_x = -std::numeric_limits<double>::infinity();
        double min_y = std::numeric_limits<double>::infinity();
        double max_y = -std::numeric_limits<double>::infinity();
        double weight_sum = 0.0;

        for (int bid : net.connected_block_ids) {
            const size_t b_idx = static_cast<size_t>(bid);
            const double cx = scratchpad.center_x[b_idx];
            const double cy = scratchpad.center_y[b_idx];

            if (cx < min_x) min_x = cx;
            if (cx > max_x) max_x = cx;
            if (cy < min_y) min_y = cy;
            if (cy > max_y) max_y = cy;

            weight_sum += context.blocks[b_idx].weight;
        }

        const double net_hpwl = (max_x - min_x) + (max_y - min_y);
        const double net_w = weight_sum / static_cast<double>(net.connected_block_ids.size());
        total_hpwl += net_w * net_hpwl;
    }

    // Нормализация компонент
    const double area_norm = (chip_w > 0.0 && chip_h > 0.0) ? (chip_w * chip_h) : 1.0;
    const double num_nets = static_cast<double>(context.nets.size());
    const double hpwl_norm = std::max(1.0, num_nets * ((chip_w + chip_h) / 2.0));

    // Итоговый фитнес
    const double w_hpwl = context.ga_config.hpwl_weight;
    const double w_area = context.ga_config.area_weight;

    const double cost = w_hpwl * (total_hpwl / hpwl_norm) +
                        w_area * (actual_area / area_norm) +
                        penalty;

    individual.fitness = cost;
    individual.raw_hpwl = total_hpwl;
    individual.raw_area = actual_area;
    individual.is_feasible = is_feasible;

    return cost;
}

} // namespace core
