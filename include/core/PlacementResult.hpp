#pragma once

#include "core/PlacedBlock.hpp"
#include "core/QualityMetrics.hpp"
#include "core/ExecutionStats.hpp"
#include "verifier/VerificationReport.hpp"

#include <vector>

namespace core {

/**
 * @brief Полный агрегированный результат работы САПР.
 */
struct PlacementResult {
    std::vector<PlacedBlock> blocks;
    QualityMetrics metrics;
    ExecutionStats stats;
    std::vector<double> fitness_history;
    verifier::VerificationReport verification;
};

} // namespace core
