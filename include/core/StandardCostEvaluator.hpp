#pragma once

#include "core/ICostEvaluator.hpp"

namespace core {

/**
 * @brief Оценка целевой HPWL и площади со штрафом за границы
 */
class StandardCostEvaluator : public ICostEvaluator {
public:
    StandardCostEvaluator() = default;
    ~StandardCostEvaluator() override = default;

    /**
     * @brief Расчет целевой функции
     * @param individual Особь
     * @param context Данные схемы
     * @param scratchpad Буфер с координатами
     * @return Значение функции стоимости
     */
    double evaluate(
        Individual& individual,
        const InputContext& context,
        const EvaluationScratchpad& scratchpad) const noexcept override;

    static constexpr double kBarrierPenalty = 10000.0;
    static constexpr double kPenaltyCoeff = 50.0;
    static constexpr double kEpsilon = 1e-7;
};

} // namespace core
