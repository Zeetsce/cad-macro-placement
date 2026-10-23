#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

#include "core/DagPlacementDecoder.hpp"
#include "core/GARunner.hpp"
#include "core/StandardCostEvaluator.hpp"
#include "io/JsonIO.hpp"
#include "io/SvgExporter.hpp"
#include "verifier/PlacementVerifier.hpp"

static double get_process_peak_memory_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return 0.0;
}

static void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  --input <path>   Path to input JSON design (default: data/sample_team1.json)\n"
              << "  --output <path>  Path to output JSON result (default: output.json)\n"
              << "  --svg <path>     Path to output SVG visualization (optional)\n"
              << "  --seed <int>     Random seed for GA reproducibility (default: 42)\n"
              << "  --help           Print this help message\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::string input_file = "data/sample_team1.json";
    std::string output_file = "output.json";
    std::string svg_file = "placement.svg";
    uint32_t seed = 42;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (arg == "--input" && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--svg" && i + 1 < argc) {
            svg_file = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    std::cout << "===================================================================\n";
    std::cout << "   САПР ТОПОЛОГИЧЕСКОГО РАЗМЕЩЕНИЯ МАКРОБЛОКОВ СБИС (КОМАНДА 1)   \n";
    std::cout << "       Sequence Pair & Genetic Algorithm Engine                   \n";
    std::cout << "===================================================================\n\n";

    // Десериализация входного JSON
    std::cout << "[1/5] Loading input design from: " << input_file << " ...\n";
    core::InputContext context;
    try {
        context = io::JsonIO::parse_input_file(input_file);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to load input file: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "      Chip dimensions: " << context.chip.width << " x " << context.chip.height
              << " (grid: " << context.chip.grid_size << ", rotation: "
              << (context.chip.rotation_allowed ? "ENABLED" : "DISABLED") << ")\n";
    std::cout << "      Macroblocks count: " << context.blocks.size() << "\n";
    std::cout << "      Nets (hyperedges) count: " << context.nets.size() << "\n";
    std::cout << "      GA settings: pop_size=" << context.ga_config.population_size
              << ", generations=" << context.ga_config.generations
              << ", crossover=" << (context.ga_config.crossover == core::CrossoverType::ORDER_CROSSOVER ? "OX" : "PMX")
              << ", seed=" << seed << "\n\n";

    // Инициализация вычислительного ядра
    std::cout << "[2/5] Initializing core engine...\n";
    core::DagPlacementDecoder decoder;
    core::StandardCostEvaluator evaluator;
    core::GARunner runner(decoder, evaluator);

    // Запуск эволюционной оптимизации
    std::cout << "[3/5] Running evolutionary placement optimization...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    core::PlacementResult result = runner.run(context, seed);
    auto end_time = std::chrono::high_resolution_clock::now();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    result.stats.elapsed_time_ms = elapsed_ms;

    const double peak_mem = get_process_peak_memory_mb();
    if (peak_mem > 0.0) {
        result.stats.peak_memory_mb = peak_mem;
    }

    std::cout << "      Optimization finished in " << elapsed_ms << " ms.\n\n";

    // Независимый аудит верификатором
    std::cout << "[4/5] Running independent geometry and connectivity audit...\n";
    verifier::PlacementVerifier verifier;
    auto report = verifier.verify(context, result.blocks, result.metrics.total_hpwl);
    result.verification = report;

    if (report.is_valid) {
        std::cout << "      [VERIFIER VERDICT]      >>> VALID TOPOLOGY <<<  \n";
    } else {
        std::cerr << "      [VERIFIER VERDICT] >>> TOPOLOGY VIOLATIONS DETECTED <<<\n";
        std::cerr << report.to_string() << "\n\n";
    }

    // Сериализация выходных данных
    std::cout << "[5/5] Exporting placement results...\n";
    try {
        io::JsonIO::write_output_file(output_file, result, context);
        std::cout << "      Output JSON saved to: " << output_file << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to write output JSON: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (!svg_file.empty()) {
        try {
            io::SvgExporter::export_to_svg(svg_file, result, context);
            std::cout << "      Vector SVG topology saved to: " << svg_file << " (+3 bonus points)\n";
        } catch (const std::exception& e) {
            std::cerr << "[WARNING] Failed to generate SVG: " << e.what() << std::endl;
        }
    }

    // Итоговый отчет телеметрии
    std::cout << "\n===================================================================\n";
    std::cout << "                      ИТОГОВЫЙ ОТЧЕТ РАЗМЕЩЕНИЯ                    \n";
    std::cout << "===================================================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Суммарная длина связей (HPWL):       " << result.metrics.total_hpwl << "\n";
    std::cout << "  Площадь размещения (Area):           " << result.metrics.total_area << " (W="
              << result.metrics.actual_width << ", H=" << result.metrics.actual_height << ")\n";
    std::cout << "  Коэффициент заполнения (Density):    " << (result.metrics.chip_utilization * 100.0) << " %\n";
    std::cout << "  Финальное значение целевой функции:  " << result.metrics.final_cost << "\n";
    std::cout << "  Время вычислений:                    " << result.stats.elapsed_time_ms << " ms\n";
    std::cout << "  Пиковая память:                      " << result.stats.peak_memory_mb << " MB\n";
    std::cout << "  Поколение сходимости:                " << result.stats.convergence_generation
              << " из " << result.stats.total_generations << "\n";
    std::cout << "  Статус верификатора:                 " << (report.is_valid ? "УСПЕШНО" : "ОШИБКА") << "\n";
    std::cout << "===================================================================\n";

    return report.is_valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
