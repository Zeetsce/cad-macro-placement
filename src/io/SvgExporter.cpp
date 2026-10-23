#include "io/SvgExporter.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace io {

static const std::vector<std::string> kBlockColors = {
    "#4A90E2", "#50E3C2", "#F5A623", "#E94E77",
    "#BD10E0", "#9013FE", "#7ED321", "#F8E71C",
    "#D0021B", "#417505", "#4A4A4A", "#1B809E"
};

void SvgExporter::export_to_svg(
    const std::string& filepath,
    const core::PlacementResult& result,
    const core::InputContext& context) {

    const std::string svg_data = generate_svg_string(result, context);
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open SVG file for writing: " + filepath);
    }
    out << svg_data;
}

std::string SvgExporter::generate_svg_string(
    const core::PlacementResult& result,
    const core::InputContext& context) {

    const double chip_w = context.chip.width;
    const double chip_h = context.chip.height;
    const double margin = 60.0;
    const double total_w = chip_w + 2.0 * margin;
    const double total_h = chip_h + 2.0 * margin + 40.0; // Дополнительное место для легенды сверху

    std::ostringstream svg;
    svg << std::fixed << std::setprecision(1);

    // SVG Header
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "width=\"" << total_w << "\" height=\"" << total_h << "\" "
        << "viewBox=\"0 0 " << total_w << " " << total_h << "\">\n";

    // Стили
    svg << "<defs>\n"
        << "  <style>\n"
        << "    .title { font-family: 'Segoe UI', Arial, sans-serif; font-size: 16px; font-weight: bold; fill: #1a1a1a; }\n"
        << "    .subtitle { font-family: 'Segoe UI', Arial, sans-serif; font-size: 13px; fill: #555; }\n"
        << "    .chip-border { fill: #fdfdfd; stroke: #222; stroke-width: 2.5; }\n"
        << "    .block-rect { stroke: #2c3e50; stroke-width: 1.5; rx: 4; fill-opacity: 0.85; }\n"
        << "    .block-label { font-family: 'Consolas', monospace; font-size: 11px; font-weight: bold; fill: #111; text-anchor: middle; }\n"
        << "    .net-line { stroke: #e74c3c; stroke-width: 1.5; stroke-dasharray: 4,4; stroke-opacity: 0.6; }\n"
        << "  </style>\n"
        << "</defs>\n\n";

    // Фон холста
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"#f5f7fa\"/>\n";

    // Заголовок и метрики (вверху)
    svg << "<text x=\"" << margin << "\" y=\"30\" class=\"title\">САПР Топологического Размещения СБИС (Sequence Pair &amp; GA)</text>\n";
    svg << "<text x=\"" << margin << "\" y=\"50\" class=\"subtitle\">"
        << "HPWL: " << result.metrics.total_hpwl << " | "
        << "Area: " << result.metrics.total_area << " | "
        << "Utilization: " << (result.metrics.chip_utilization * 100.0) << "% | "
        << "Blocks: " << result.blocks.size() << " | "
        << "Nets: " << context.nets.size()
        << "</text>\n\n";

    const double origin_x = margin;
    const double origin_y = margin + 40.0;

    // Контур кристалла (Chip Boundary)
    svg << "<rect x=\"" << origin_x << "\" y=\"" << origin_y << "\" "
        << "width=\"" << chip_w << "\" height=\"" << chip_h << "\" "
        << "class=\"chip-border\"/>\n\n";

    // Отрисовка гиперребер (Nets) - линии связей между геометрическими центрами
    svg << "<!-- Электрические цепи (межсоединения) -->\n";
    for (const auto& net : context.nets) {
        if (net.connected_block_ids.size() <= 1) continue;

        // Связываем последовательно или через звездный центр
        for (size_t i = 0; i < net.connected_block_ids.size(); ++i) {
            for (size_t j = i + 1; j < net.connected_block_ids.size(); ++j) {
                const int bid_a = net.connected_block_ids[i];
                const int bid_b = net.connected_block_ids[j];
                if (bid_a < 0 || bid_b < 0 ||
                    static_cast<size_t>(bid_a) >= result.blocks.size() ||
                    static_cast<size_t>(bid_b) >= result.blocks.size()) {
                    continue;
                }

                const auto& b1 = result.blocks[static_cast<size_t>(bid_a)];
                const auto& b2 = result.blocks[static_cast<size_t>(bid_b)];

                // Преобразование координат Y: САПР снизу-вверх -> SVG сверху-вниз
                const double x1 = origin_x + b1.center_x;
                const double y1 = origin_y + (chip_h - b1.center_y);
                const double x2 = origin_x + b2.center_x;
                const double y2 = origin_y + (chip_h - b2.center_y);

                svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" "
                    << "x2=\"" << x2 << "\" y2=\"" << y2 << "\" "
                    << "class=\"net-line\"/>\n";
            }
        }
    }
    svg << "\n";

    // Отрисовка размещенных макроблоков
    svg << "<!-- Размещенные макроблоки -->\n";
    for (size_t idx = 0; idx < result.blocks.size(); ++idx) {
        const auto& b = result.blocks[idx];
        const double bx = origin_x + b.x;
        const double by = origin_y + (chip_h - (b.y + b.height)); // Инверсия Y для SVG

        const std::string color = kBlockColors[idx % kBlockColors.size()];
        const std::string name = (b.block_id >= 0 && static_cast<size_t>(b.block_id) < context.index_to_block_name.size())
            ? context.index_to_block_name[static_cast<size_t>(b.block_id)]
            : std::to_string(b.block_id);

        svg << "<g id=\"block_" << b.block_id << "\">\n";
        svg << "  <rect x=\"" << bx << "\" y=\"" << by << "\" "
            << "width=\"" << b.width << "\" height=\"" << b.height << "\" "
            << "fill=\"" << color << "\" class=\"block-rect\"/>\n";

        // Текстовая метка внутри блока
        const double text_x = bx + (b.width / 2.0);
        const double text_y = by + (b.height / 2.0) + 4.0;
        svg << "  <text x=\"" << text_x << "\" y=\"" << text_y << "\" class=\"block-label\">"
            << name << (b.is_rotated ? " (R)" : "") << "</text>\n";
        svg << "</g>\n";
    }

    svg << "</svg>\n";
    return svg.str();
}

} // namespace io
