#pragma once

#include "core/InputContext.hpp"
#include "core/PlacedBlock.hpp"
#include "verifier/VerificationReport.hpp"

#include <vector>

namespace verifier {

/**
 * @brief Верификатор геометрии размещения блоков
 */
class PlacementVerifier {
public:
    PlacementVerifier() = default;
    ~PlacementVerifier() = default;

    /**
     * @brief Полная проверка размещения
     * @param context Данные схемы
     * @param blocks Размещенные блоки
     * @param claimed_hpwl Заявленное значение HPWL
     * @return Отчет верификации
     */
    VerificationReport verify(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks,
        double claimed_hpwl) const;

    /**
     * @brief Экспресс-проверка геометрии
     * @param context Данные схемы
     * @param blocks Размещенные блоки
     * @return true если нет пересечений и выхода за границы
     */
    bool is_geometrically_valid(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks) const;

private:
    static constexpr double kEpsilon = 1e-7;

    void check_overlaps(
        const std::vector<core::PlacedBlock>& blocks,
        VerificationReport& report) const;

    void check_boundaries_and_grid(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks,
        VerificationReport& report) const;

    void check_rotations(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks,
        VerificationReport& report) const;

    void check_completeness(
        size_t expected_count,
        const std::vector<core::PlacedBlock>& blocks,
        VerificationReport& report) const;

    double compute_independent_hpwl(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks) const;
};

} // namespace verifier
