#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "io/JsonIO.hpp"
#include "nlohmann/json.hpp"

static bool test_parse_sample_file() {
    try {
        std::string sample_path = "data/sample_team1.json";
        std::ifstream check(sample_path);
        if (!check.good()) {
            sample_path = "../data/sample_team1.json";
        }
        const auto context = io::JsonIO::parse_input_file(sample_path);

        if (context.chip.width != 1000.0 || context.chip.height != 800.0 ||
            context.chip.grid_size != 10.0 || !context.chip.rotation_allowed) {
            std::cerr << "[FAIL] Parsed chip spec mismatch!" << std::endl;
            return false;
        }

        if (context.blocks.size() != 3) {
            std::cerr << "[FAIL] Blocks count mismatch: " << context.blocks.size() << std::endl;
            return false;
        }

        if (context.block_name_to_index.at("Cell1") != 0 ||
            context.block_name_to_index.at("Cell2") != 1 ||
            context.block_name_to_index.at("Cell3") != 2) {
            std::cerr << "[FAIL] NameTable mapping mismatch!" << std::endl;
            return false;
        }

        if (context.nets.size() != 2) {
            std::cerr << "[FAIL] Nets count mismatch: " << context.nets.size() << std::endl;
            return false;
        }

        if (context.nets[0].connected_block_ids != std::vector<int>{0, 1} ||
            context.nets[1].connected_block_ids != std::vector<int>{1, 2}) {
            std::cerr << "[FAIL] Net connections mapping mismatch!" << std::endl;
            return false;
        }

        if (context.ga_config.population_size != 60 ||
            context.ga_config.generations != 120 ||
            context.ga_config.crossover != core::CrossoverType::ORDER_CROSSOVER ||
            context.ga_config.selection != core::SelectionMethod::TOURNAMENT) {
            std::cerr << "[FAIL] GA config mismatch!" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Exception during sample file parsing: " << e.what() << std::endl;
        return false;
    }

    return true;
}

static bool test_output_serialization() {
    core::InputContext context;
    context.index_to_block_name = {"MacroA", "MacroB"};
    context.block_name_to_index["MacroA"] = 0;
    context.block_name_to_index["MacroB"] = 1;

    core::PlacementResult result;
    result.blocks = {
        {0, 0.0, 0.0, 100.0, 50.0, 50.0, 25.0, false},
        {1, 100.0, 0.0, 60.0, 40.0, 130.0, 20.0, true}
    };
    result.metrics.total_hpwl = 80.0;
    result.metrics.total_area = 8000.0;
    result.metrics.actual_width = 160.0;
    result.metrics.actual_height = 50.0;
    result.metrics.chip_utilization = 0.92;
    result.metrics.final_cost = 0.85;

    result.stats.elapsed_time_ms = 150;
    result.stats.peak_memory_mb = 5.2;
    result.stats.total_generations = 100;
    result.stats.convergence_generation = 64;

    result.fitness_history = {500.0, 300.0, 150.0, 80.0};

    const std::string json_str = io::JsonIO::write_output_string(result, context);

    try {
        const auto root = nlohmann::json::parse(json_str);

        // Проверка верхнеуровневых полей по ТЗ
        if (!root.contains("hpwl") || std::abs(root["hpwl"].get<double>() - 80.0) > 1e-5) {
            std::cerr << "[FAIL] Top-level 'hpwl' missing or wrong!" << std::endl;
            return false;
        }
        if (!root.contains("area") || std::abs(root["area"].get<double>() - 8000.0) > 1e-5) {
            std::cerr << "[FAIL] Top-level 'area' missing or wrong!" << std::endl;
            return false;
        }
        if (!root.contains("fitness_history") || root["fitness_history"].size() != 4) {
            std::cerr << "[FAIL] Top-level 'fitness_history' missing or size mismatch!" << std::endl;
            return false;
        }
        if (!root.contains("blocks") || root["blocks"].size() != 2) {
            std::cerr << "[FAIL] 'blocks' array missing or size mismatch!" << std::endl;
            return false;
        }
        if (!root.contains("placement") || root["placement"].size() != 2) {
            std::cerr << "[FAIL] 'placement' array missing or size mismatch!" << std::endl;
            return false;
        }

        // Проверка имён блоков
        if (root["blocks"][0]["id"] != "MacroA" || root["blocks"][1]["id"] != "MacroB") {
            std::cerr << "[FAIL] Block name serialization mismatch!" << std::endl;
            return false;
        }

        // Проверка детальной телеметрии
        if (root["telemetry"]["memory_mb"].get<double>() != 5.2 ||
            root["telemetry"]["convergence_generation"].get<int>() != 64) {
            std::cerr << "[FAIL] Telemetry values mismatch!" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Failed to parse generated output JSON: " << e.what() << std::endl;
        return false;
    }

    return true;
}

static bool test_error_handling() {
    // Невалидный JSON
    try {
        io::JsonIO::parse_input_string("{ invalid json syntax }");
        std::cerr << "[FAIL] Expected exception for syntax error, but none was thrown!" << std::endl;
        return false;
    } catch (const std::runtime_error&) {
        // Ожидаемо
    }

    // Ссылка на несуществующий блок в цепи
    const std::string bad_net_json = R"({
        "design": { "chip_width": 100, "chip_height": 100 },
        "blocks": [ { "id": "B1", "width": 10, "height": 10 } ],
        "nets": [ { "net_id": "N1", "block_ids": ["B1", "UNKNOWN_BLOCK"] } ]
    })";

    try {
        io::JsonIO::parse_input_string(bad_net_json);
        std::cerr << "[FAIL] Expected exception for unknown block in net, but none was thrown!" << std::endl;
        return false;
    } catch (const std::runtime_error&) {
        // Ожидаемо
    }

    // Отсутствие секции design
    const std::string missing_design_json = R"({
        "blocks": [ { "id": "B1", "width": 10, "height": 10 } ]
    })";

    try {
        io::JsonIO::parse_input_string(missing_design_json);
        std::cerr << "[FAIL] Expected exception for missing design, but none was thrown!" << std::endl;
        return false;
    } catch (const std::runtime_error&) {
        // Ожидаемо
    }

    return true;
}

int main() {
    std::cout << "[RUN] Running test_json_io..." << std::endl;

    if (!test_parse_sample_file()) return EXIT_FAILURE;
    if (!test_output_serialization()) return EXIT_FAILURE;
    if (!test_error_handling()) return EXIT_FAILURE;

    std::cout << "[PASS] All test_json_io scenarios passed successfully." << std::endl;
    return EXIT_SUCCESS;
}
