#include <cstdlib>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

/**
 * @brief Тестирование базовых возможностей C++17 и доступности nlohmann/json.
 *
 * @return int Код завершения теста: EXIT_SUCCESS в случае успеха, EXIT_FAILURE при ошибке.
 *
 * @pre Библиотека nlohmann/json корректно подключена в CMake.
 * @post Проверены базовая сериализация и десериализация JSON-объекта.
 *
 * @note Time complexity: O(1)
 * @note Space complexity: O(1)
 */
int main() {
    std::cout << "[RUN] Running test_skeleton..." << std::endl;

    // Проверка создания и сериализации nlohmann::json
    nlohmann::json test_doc;
    test_doc["project"] = "cad_macro_placement";
    test_doc["version"] = 1;
    test_doc["status"] = "ok";
    test_doc["blocks"] = nlohmann::json::array({1, 2, 3});

    const std::string serialized = test_doc.dump();
    if (serialized.empty()) {
        std::cerr << "[FAIL] Serialized JSON string is empty!" << std::endl;
        return EXIT_FAILURE;
    }

    // Проверка парсинга и валидации полей
    try {
        const auto parsed_doc = nlohmann::json::parse(serialized);

        if (!parsed_doc.contains("project") || parsed_doc["project"] != "cad_macro_placement") {
            std::cerr << "[FAIL] Field 'project' mismatch or missing!" << std::endl;
            return EXIT_FAILURE;
        }

        if (!parsed_doc.contains("version") || parsed_doc["version"] != 1) {
            std::cerr << "[FAIL] Field 'version' mismatch or missing!" << std::endl;
            return EXIT_FAILURE;
        }

        if (!parsed_doc.contains("blocks") || parsed_doc["blocks"].size() != 3) {
            std::cerr << "[FAIL] Field 'blocks' array size mismatch!" << std::endl;
            return EXIT_FAILURE;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] Exception caught during JSON parsing: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[PASS] test_skeleton passed successfully. JSON library is operational." << std::endl;
    return EXIT_SUCCESS;
}
