#pragma once

#include "core/BlockData.hpp"
#include "core/ChipSpec.hpp"
#include "core/GAConfig.hpp"
#include "core/NetData.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace core {

/**
 * @brief Контекст задачи размещения
 */
struct InputContext {
    ChipSpec chip;
    std::vector<BlockData> blocks;
    std::vector<NetData> nets;
    GAConfig ga_config;

    std::vector<std::string> index_to_block_name;
    std::unordered_map<std::string, int> block_name_to_index;
    std::vector<std::string> index_to_net_name;
};

} // namespace core
