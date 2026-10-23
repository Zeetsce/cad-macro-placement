#pragma once

namespace core {

/**
 * @brief Неизменяемые технологические характеристики макроблока.
 */
struct BlockData {
    int id = -1;
    double width = 0.0;            // Номинальная ширина блока (width > 0)
    double height = 0.0;           // Номинальная высота блока (height > 0)
    double weight = 1.0;           // Весовой коэффициент блока в целевой функции (> 0)
    bool rotation_allowed = false; // Разрешение на поворот данного конкретного блока
};

} // namespace core
