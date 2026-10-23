#pragma once

#include "core/IPlacementDecoder.hpp"

namespace core {

/**
 * @brief Декодер Sequence Pair в координаты через граф
 */
class DagPlacementDecoder : public IPlacementDecoder {
public:
    DagPlacementDecoder() = default;
    ~DagPlacementDecoder() override = default;

    /**
     * @brief Расчет координат блоков в буфер
     * @param individual Особь
     * @param context Данные схемы
     * @param scratchpad Буфер координат
     */
    void decode_to_scratchpad(
        const Individual& individual,
        const InputContext& context,
        EvaluationScratchpad& scratchpad) const noexcept override;

    /**
     * @brief Получение списка размещенных блоков
     * @param individual Особь
     * @param context Данные схемы
     * @return Вектор блоков с координатами
     */
    std::vector<PlacedBlock> decode_to_layout(
        const Individual& individual,
        const InputContext& context) const override;
};

} // namespace core
