#pragma once

#include "core/InputContext.hpp"
#include "core/PlacementResult.hpp"

#include <cstdint>

namespace core {

/**
 * @brief Интерфейс алгоритма оптимизации размещения
 */
class IGAEngine {
public:
    virtual ~IGAEngine() = default;

    /**
     * @brief Запуск оптимизации
     * @param context Данные схемы
     * @param seed Зерно генератора случайных чисел
     * @return Результат размещения и метрики
     */
    virtual PlacementResult run(const InputContext& context, uint32_t seed = 42) = 0;
};

} // namespace core
