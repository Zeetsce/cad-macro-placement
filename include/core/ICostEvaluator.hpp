#pragma once

#include "core/EvaluationScratchpad.hpp"
#include "core/Individual.hpp"
#include "core/InputContext.hpp"

namespace core {

/**
 * @brief Интерфейс вычисления функции стоимости
 */
class ICostEvaluator {
public:
    virtual ~ICostEvaluator() = default;

    /**
     * @brief Расчет фитнеса особи по координатам из буфера
     * @param individual Особь
     * @param context Данные схемы
     * @param scratchpad Буфер с координатами
     * @return Значение функции стоимости
     */
    virtual double evaluate(
        Individual& individual,
        const InputContext& context,
        const EvaluationScratchpad& scratchpad) const noexcept = 0;
};

} // namespace core
