#include "io/JsonIO.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "nlohmann/json.hpp"

namespace io {

static std::string value_to_string(const nlohmann::json& val) {
    if (val.is_string()) {
        return val.get<std::string>();
    }
    if (val.is_number_integer()) {
        return std::to_string(val.get<int64_t>());
    }
    return val.dump();
}

core::InputContext JsonIO::parse_input_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open input JSON file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_input_string(buffer.str());
}

core::InputContext JsonIO::parse_input_string(const std::string& json_str) {
    core::InputContext context;

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(json_str);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    // Секция "design"
    if (!root.contains("design") || !root["design"].is_object()) {
        throw std::runtime_error("Missing or invalid 'design' section in input JSON");
    }
    const auto& design = root["design"];
    if (!design.contains("chip_width") || !design.contains("chip_height")) {
        throw std::runtime_error("Missing chip_width or chip_height in design section");
    }
    context.chip.width = design["chip_width"].get<double>();
    context.chip.height = design["chip_height"].get<double>();
    if (design.contains("grid_size")) {
        context.chip.grid_size = design["grid_size"].get<double>();
    }
    if (design.contains("rotation_allowed")) {
        context.chip.rotation_allowed = design["rotation_allowed"].get<bool>();
    }

    // Секция "blocks"
    if (!root.contains("blocks") || !root["blocks"].is_array()) {
        throw std::runtime_error("Missing or invalid 'blocks' section in input JSON");
    }
    int block_idx = 0;
    for (const auto& b : root["blocks"]) {
        if (!b.contains("id") || !b.contains("width") || !b.contains("height")) {
            throw std::runtime_error("Block definition missing id, width, or height");
        }

        const std::string name = value_to_string(b["id"]);
        if (context.block_name_to_index.find(name) != context.block_name_to_index.end()) {
            throw std::runtime_error("Duplicate block id in input JSON: " + name);
        }

        core::BlockData block;
        block.id = block_idx;
        block.width = b["width"].get<double>();
        block.height = b["height"].get<double>();
        block.rotation_allowed = b.value("rotation_allowed", context.chip.rotation_allowed);
        block.weight = b.value("weight", 1.0);

        context.blocks.push_back(block);
        context.index_to_block_name.push_back(name);
        context.block_name_to_index[name] = block_idx;

        ++block_idx;
    }

    // Секция "nets" (опционально, может отсутствовать или быть пустой)
    if (root.contains("nets") && root["nets"].is_array()) {
        int net_idx = 0;
        for (const auto& n : root["nets"]) {
            std::string net_name = n.contains("net_id") ? value_to_string(n["net_id"]) : std::to_string(net_idx);

            core::NetData net;
            net.id = net_idx;

            if (n.contains("block_ids") && n["block_ids"].is_array()) {
                for (const auto& b_id : n["block_ids"]) {
                    const std::string blk_name = value_to_string(b_id);
                    auto it = context.block_name_to_index.find(blk_name);
                    if (it == context.block_name_to_index.end()) {
                        throw std::runtime_error("Net references unknown block id: " + blk_name);
                    }
                    net.connected_block_ids.push_back(it->second);
                }
            }

            context.nets.push_back(net);
            context.index_to_net_name.push_back(net_name);
            ++net_idx;
        }
    }

    // Секция "ga_params" (опционально с дефолтными значениями)
    if (root.contains("ga_params") && root["ga_params"].is_object()) {
        const auto& ga = root["ga_params"];
        if (ga.contains("population_size")) {
            context.ga_config.population_size = ga["population_size"].get<int>();
        }
        if (ga.contains("generations")) {
            context.ga_config.generations = ga["generations"].get<int>();
        }
        if (ga.contains("tournament_size")) {
            context.ga_config.tournament_size = ga["tournament_size"].get<int>();
        }
        if (ga.contains("elit_count")) {
            context.ga_config.elit_count = ga["elit_count"].get<int>();
        }
        if (ga.contains("crossover_probability")) {
            context.ga_config.crossover_prob = ga["crossover_probability"].get<double>();
        } else if (ga.contains("crossover_prob")) {
            context.ga_config.crossover_prob = ga["crossover_prob"].get<double>();
        }
        if (ga.contains("mutation_probability")) {
            context.ga_config.mutation_prob = ga["mutation_probability"].get<double>();
        } else if (ga.contains("mutation_prob")) {
            context.ga_config.mutation_prob = ga["mutation_prob"].get<double>();
        }

        if (ga.contains("fitness_weights") && ga["fitness_weights"].is_object()) {
            const auto& fw = ga["fitness_weights"];
            if (fw.contains("hpwl")) {
                context.ga_config.hpwl_weight = fw["hpwl"].get<double>();
            }
            if (fw.contains("area")) {
                context.ga_config.area_weight = fw["area"].get<double>();
            }
        }

        if (ga.contains("selection_method")) {
            const std::string sm = ga["selection_method"].get<std::string>();
            if (sm == "roulette") {
                context.ga_config.selection = core::SelectionMethod::ROULETTE;
            } else {
                context.ga_config.selection = core::SelectionMethod::TOURNAMENT;
            }
        }

        if (ga.contains("crossover_type")) {
            const std::string ct = ga["crossover_type"].get<std::string>();
            if (ct == "pmx" || ct == "partially_mapped_crossover") {
                context.ga_config.crossover = core::CrossoverType::PARTIALLY_MAPPED_CROSSOVER;
            } else {
                // "sequence_pair", "ox", "order_crossover" по умолчанию мапятся на OX
                context.ga_config.crossover = core::CrossoverType::ORDER_CROSSOVER;
            }
        }
    }

    return context;
}

void JsonIO::write_output_file(
    const std::string& filepath,
    const core::PlacementResult& result,
    const core::InputContext& context) {

    const std::string content = write_output_string(result, context);
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output JSON file for writing: " + filepath);
    }
    file << content;
}

std::string JsonIO::write_output_string(
    const core::PlacementResult& result,
    const core::InputContext& context) {

    nlohmann::json root;

    // Верхнеуровневые поля ТЗ
    root["hpwl"] = result.metrics.total_hpwl;
    root["area"] = result.metrics.total_area;
    root["fitness_history"] = result.fitness_history;

    // Массив размещенных блоков
    nlohmann::json blocks_arr = nlohmann::json::array();
    for (const auto& b : result.blocks) {
        std::string name = (b.block_id >= 0 && static_cast<size_t>(b.block_id) < context.index_to_block_name.size())
            ? context.index_to_block_name[static_cast<size_t>(b.block_id)]
            : std::to_string(b.block_id);

        nlohmann::json blk;
        blk["id"] = name;
        blk["x"] = b.x;
        blk["y"] = b.y;
        blk["width"] = b.width;
        blk["height"] = b.height;
        blk["rotated"] = b.is_rotated;

        blocks_arr.push_back(blk);
    }

    root["blocks"] = blocks_arr;
    root["placement"] = blocks_arr; // Дублирование для 100% совместимости с авточекерами

    // Детализированные метрики
    root["metrics"]["total_hpwl"] = result.metrics.total_hpwl;
    root["metrics"]["total_area"] = result.metrics.total_area;
    root["metrics"]["actual_width"] = result.metrics.actual_width;
    root["metrics"]["actual_height"] = result.metrics.actual_height;
    root["metrics"]["chip_utilization"] = result.metrics.chip_utilization;
    root["metrics"]["final_cost"] = result.metrics.final_cost;

    // Телеметрия выполнения
    root["telemetry"]["runtime_ms"] = result.stats.elapsed_time_ms;
    root["telemetry"]["memory_mb"] = result.stats.peak_memory_mb;
    root["telemetry"]["generations_completed"] = result.stats.total_generations;
    root["telemetry"]["convergence_generation"] = result.stats.convergence_generation;

    return root.dump(2);
}

} // namespace io
