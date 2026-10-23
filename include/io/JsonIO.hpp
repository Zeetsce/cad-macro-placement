#pragma once

#include "core/InputContext.hpp"
#include "core/PlacementResult.hpp"

#include <string>

namespace io {

/**
 * @brief Чтение и запись данных в формате JSON
 */
class JsonIO {
public:
    /**
     * @brief Загрузка входного файла задачи
     * @param filepath Путь к файлу
     * @return Контекст задачи
     */
    static core::InputContext parse_input_file(const std::string& filepath);

    /**
     * @brief Парсинг JSON из строки
     * @param json_str Строка с данными
     * @return Контекст задачи
     */
    static core::InputContext parse_input_string(const std::string& json_str);

    /**
     * @brief Запись результата размещения в файл
     * @param filepath Путь к файлу
     * @param result Результаты оптимизации
     * @param context Контекст задачи
     */
    static void write_output_file(
        const std::string& filepath,
        const core::PlacementResult& result,
        const core::InputContext& context);

    /**
     * @brief Сериализация результатов в строку JSON
     * @param result Результаты оптимизации
     * @param context Контекст задачи
     * @return Строка JSON
     */
    static std::string write_output_string(
        const core::PlacementResult& result,
        const core::InputContext& context);
};

} // namespace io
