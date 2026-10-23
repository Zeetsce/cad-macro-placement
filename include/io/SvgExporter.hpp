#pragma once

#include "core/InputContext.hpp"
#include "core/PlacementResult.hpp"

#include <string>

namespace io {

/**
 * @brief Экспорт топологии в формат SVG
 */
class SvgExporter {
public:
    /**
     * @brief Экспорт чертежа в файл SVG
     * @param filepath Путь к файлу
     * @param result Результаты размещения
     * @param context Данные схемы
     */
    static void export_to_svg(
        const std::string& filepath,
        const core::PlacementResult& result,
        const core::InputContext& context);

    /**
     * @brief Генерация строки SVG
     * @param result Результаты размещения
     * @param context Данные схемы
     * @return Текст SVG документа
     */
    static std::string generate_svg_string(
        const core::PlacementResult& result,
        const core::InputContext& context);
};

} // namespace io
