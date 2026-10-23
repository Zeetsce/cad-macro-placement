#pragma once

#include "core/EvaluationScratchpad.hpp"
#include "core/Individual.hpp"
#include "core/InputContext.hpp"
#include "core/PlacedBlock.hpp"

#include <vector>

namespace core {

/**
 * @brief Интерфейс декодера Sequence Pair
 */
class IPlacementDecoder {
public:
    virtual ~IPlacementDecoder() = default;

    /**
     * @brief Расчет координат в буфер
     * @param individual Особь
     * @param context Данные схемы
     * @param scratchpad Буфер координат
     */
    virtual void decode_to_scratchpad(
        const Individual& individual,
        const InputContext& context,
        EvaluationScratchpad& scratchpad) const noexcept = 0;

    /**
     * @brief Получение списка размещенных блоков
     * @param individual Особь
     * @param context Данные схемы
     * @return Вектор блоков с координатами
     */
    virtual std::vector<PlacedBlock> decode_to_layout(
        const Individual& individual,
        const InputContext& context) const = 0;
};

} // namespace core
