#pragma once

namespace core {

/**
 * @brief Физически материализованный блок на топологическом чертеже.
 */
struct PlacedBlock {
    int block_id = -1;

    double x = 0.0;          // Левый нижний угол
    double y = 0.0;          // Левый нижний угол
    double width = 0.0;      // Фактическая ширина с учетом поворота
    double height = 0.0;     // Фактическая высота с учетом поворота

    double center_x = 0.0;   // Геометрический центр X
    double center_y = 0.0;   // Геометрический центр Y

    bool is_rotated = false; // Фактическое состояние поворота
};

} // namespace core
