#include "core/DagPlacementDecoder.hpp"

#include <cmath>

namespace core {

void DagPlacementDecoder::decode_to_scratchpad(
    const Individual& individual,
    const InputContext& context,
    EvaluationScratchpad& scratchpad) const noexcept {

    const size_t n = context.blocks.size();
    if (n == 0) {
        return;
    }

    // Определение эффективных габаритов с учетом поворота
    for (size_t i = 0; i < n; ++i) {
        const bool rot = (individual.is_rotated[i] != 0);
        scratchpad.w[i] = rot ? context.blocks[i].height : context.blocks[i].width;
        scratchpad.h[i] = rot ? context.blocks[i].width : context.blocks[i].height;
        scratchpad.x[i] = 0.0;
        scratchpad.y[i] = 0.0;
    }

    // Построение обратных отображений позиций за O(N)
    for (size_t k = 0; k < n; ++k) {
        scratchpad.position_in_plus[static_cast<size_t>(individual.gamma_plus[k])] = static_cast<int>(k);
        scratchpad.position_in_minus[static_cast<size_t>(individual.gamma_minus[k])] = static_cast<int>(k);
    }

    const double grid = context.chip.grid_size;

    // Горизонтальный длиннейший путь (координата X)
    // Обход в порядке Gamma+
    for (size_t k = 0; k < n; ++k) {
        const int b = individual.gamma_plus[k];
        const int pos_minus_b = scratchpad.position_in_minus[static_cast<size_t>(b)];
        double max_x = 0.0;

        for (size_t j = 0; j < k; ++j) {
            const int a = individual.gamma_plus[j];
            if (scratchpad.position_in_minus[static_cast<size_t>(a)] < pos_minus_b) {
                const double cand_x = scratchpad.x[static_cast<size_t>(a)] + scratchpad.w[static_cast<size_t>(a)];
                if (cand_x > max_x) {
                    max_x = cand_x;
                }
            }
        }

        if (grid > 1e-7) {
            max_x = std::ceil(max_x / grid) * grid;
        }
        scratchpad.x[static_cast<size_t>(b)] = max_x;
    }

    // Вертикальный длиннейший путь (координата Y)
    // Обход в порядке Gamma-
    for (size_t k = 0; k < n; ++k) {
        const int a = individual.gamma_minus[k];
        const int pos_plus_a = scratchpad.position_in_plus[static_cast<size_t>(a)];
        double max_y = 0.0;

        for (size_t j = 0; j < k; ++j) {
            const int b = individual.gamma_minus[j];
            if (scratchpad.position_in_plus[static_cast<size_t>(b)] > pos_plus_a) {
                const double cand_y = scratchpad.y[static_cast<size_t>(b)] + scratchpad.h[static_cast<size_t>(b)];
                if (cand_y > max_y) {
                    max_y = cand_y;
                }
            }
        }

        if (grid > 1e-7) {
            max_y = std::ceil(max_y / grid) * grid;
        }
        scratchpad.y[static_cast<size_t>(a)] = max_y;
    }

    // Расчет центров блоков для HPWL
    for (size_t i = 0; i < n; ++i) {
        scratchpad.center_x[i] = scratchpad.x[i] + (scratchpad.w[i] / 2.0);
        scratchpad.center_y[i] = scratchpad.y[i] + (scratchpad.h[i] / 2.0);
    }
}

std::vector<PlacedBlock> DagPlacementDecoder::decode_to_layout(
    const Individual& individual,
    const InputContext& context) const {

    const size_t n = context.blocks.size();
    EvaluationScratchpad scratchpad;
    scratchpad.resize(n);

    decode_to_scratchpad(individual, context, scratchpad);

    std::vector<PlacedBlock> layout;
    layout.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        PlacedBlock pb;
        pb.block_id = static_cast<int>(i);
        pb.x = scratchpad.x[i];
        pb.y = scratchpad.y[i];
        pb.width = scratchpad.w[i];
        pb.height = scratchpad.h[i];
        pb.center_x = scratchpad.center_x[i];
        pb.center_y = scratchpad.center_y[i];
        pb.is_rotated = (individual.is_rotated[i] != 0);
        layout.push_back(pb);
    }

    return layout;
}

} // namespace core
