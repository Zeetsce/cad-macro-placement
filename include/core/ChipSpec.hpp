#pragma once

namespace core {

/**
 * @brief Параметры кристалла и глобальные технологические ограничения.
 */
struct ChipSpec {
    double width = 0.0;            // Допустимая ширина кристалла (chip_width > 0)
    double height = 0.0;           // Допустимая высота кристалла (chip_height > 0)
    double grid_size = 1.0;        // Шаг сетки трассировки/размещения (grid_size > 0)
    bool rotation_allowed = false; // Master-флаг разрешения поворота на кристалле
};

} // namespace core
