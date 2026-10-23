#pragma once

#include <vector>

namespace core {

/**
 * @brief Гиперребро схемы (список связываемых блоков).
 */
struct NetData {
    int id = -1;                          // Индекс цепи [0 .. M-1]
    std::vector<int> connected_block_ids; // Индексы подключенных блоков [0 .. N-1]
};

} // namespace core
