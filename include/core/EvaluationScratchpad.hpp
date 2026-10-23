#pragma once

#include <vector>
#include <cstddef>

namespace core {

/**
 * @brief Предвыделенный плоский буфер для декодирования и оценки без динамических аллокаций
 * @note Выделяется 1 экземпляр на рабочий поток
 */
struct EvaluationScratchpad {
    std::vector<double> x; // Координаты x левых нижних углов
    std::vector<double> y; // Координаты y левых нижних углов
    std::vector<double> w; // Фактические ширины с учётом поворота
    std::vector<double> h; // Фактические высоты с учётом поворота

    std::vector<double> center_x; // Предвычисленные центры
    std::vector<double> center_y; // Предвычисленные центры

    // Вспомогательные буферы
    std::vector<int> position_in_plus;  // Индекс блока в g+
    std::vector<int> position_in_minus; // Индекс блока в g-
    std::vector<double> length_buffer;  // Динамическое программирование длиннейшего пути

    void resize(size_t block_count) {
        x.resize(block_count);
        y.resize(block_count);
        w.resize(block_count);
        h.resize(block_count);
        center_x.resize(block_count);
        center_y.resize(block_count);
        position_in_plus.resize(block_count);
        position_in_minus.resize(block_count);
        length_buffer.resize(block_count);
    }
};

} // namespace core
