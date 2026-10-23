#pragma once

namespace core {

/**
 * @brief метрики качества итогового размещения.
 */
struct QualityMetrics {
    double total_hpwl = 0.0;       // Суммарный HPWL
    double total_area = 0.0;       // Площадь охватывающего прямоугольника
    double actual_width = 0.0;
    double actual_height = 0.0;
    double chip_utilization = 0.0; // Суммарная площадь блоков
    double final_cost = 0.0;       // Значение взвешенной целевой функции
};

} // namespace core
