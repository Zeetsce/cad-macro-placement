#include "core/GARunner.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace core {

GARunner::GARunner(const IPlacementDecoder& decoder, const ICostEvaluator& evaluator)
    : decoder_(decoder), evaluator_(evaluator) {}

PlacementResult GARunner::run(const InputContext& context, uint32_t seed) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::mt19937 rng(seed);
    PopulationState state;
    initialize_population(state, context, rng);

    // Первичная параллельная оценка
    evaluate_population_parallel(state.individuals, context);

    // Инициализация лучшей особи
    size_t best_idx = 0;
    for (size_t i = 1; i < state.individuals.size(); ++i) {
        if (state.individuals[i].fitness < state.individuals[best_idx].fitness) {
            best_idx = i;
        }
    }
    state.best_ever = state.individuals[best_idx];
    state.best_fitness_history.push_back(state.best_ever.fitness);

    const int pop_size = context.ga_config.population_size;
    const int total_gen = context.ga_config.generations;
    const int elit_count = std::max(1, std::min(context.ga_config.elit_count, pop_size));
    const double p_cross = context.ga_config.crossover_prob;
    const double p_mut = context.ga_config.mutation_prob;
    const size_t num_blocks = context.blocks.size();

    std::uniform_real_distribution<double> real_dist(0.0, 1.0);

    // Эволюционный цикл
    for (int gen = 1; gen < total_gen; ++gen) {
        state.current_generation = gen;

        // Сортировка текущего поколения по возрастанию фитнеса (минимизация)
        std::vector<size_t> rank(static_cast<size_t>(pop_size));
        std::iota(rank.begin(), rank.end(), 0);
        std::sort(rank.begin(), rank.end(), [&](size_t a, size_t b) {
            return state.individuals[a].fitness < state.individuals[b].fitness;
        });

        // Элитизм: копирование лучших особей
        for (int e = 0; e < elit_count; ++e) {
            state.next_generation[static_cast<size_t>(e)] = state.individuals[rank[static_cast<size_t>(e)]];
        }

        // Рождение потомков
        for (int i = elit_count; i < pop_size; ++i) {
            const int p1_idx = tournament_select(state.individuals, context.ga_config.tournament_size, rng);
            const int p2_idx = tournament_select(state.individuals, context.ga_config.tournament_size, rng);

            auto& child = state.next_generation[static_cast<size_t>(i)];
            const auto& p1 = state.individuals[static_cast<size_t>(p1_idx)];
            const auto& p2 = state.individuals[static_cast<size_t>(p2_idx)];

            // Кроссовер
            if (real_dist(rng) < p_cross) {
                if (context.ga_config.crossover == CrossoverType::PARTIALLY_MAPPED_CROSSOVER) {
                    pmx_crossover(p1.gamma_plus, p2.gamma_plus, child.gamma_plus, rng);
                    pmx_crossover(p1.gamma_minus, p2.gamma_minus, child.gamma_minus, rng);
                } else {
                    order_crossover(p1.gamma_plus, p2.gamma_plus, child.gamma_plus, rng);
                    order_crossover(p1.gamma_minus, p2.gamma_minus, child.gamma_minus, rng);
                }

                // Наследование флагов поворота
                for (size_t b = 0; b < num_blocks; ++b) {
                    child.is_rotated[b] = (real_dist(rng) < 0.5) ? p1.is_rotated[b] : p2.is_rotated[b];
                }
            } else {
                child.gamma_plus = p1.gamma_plus;
                child.gamma_minus = p1.gamma_minus;
                child.is_rotated = p1.is_rotated;
            }

            // Мутации
            if (real_dist(rng) < p_mut) {
                mutate_swap(child.gamma_plus, rng);
            }
            if (real_dist(rng) < p_mut) {
                mutate_swap(child.gamma_minus, rng);
            }
            if (real_dist(rng) < p_mut) {
                mutate_rotate(child.is_rotated, context, rng);
            }
        }

        // Параллельная оценка нового поколения
        evaluate_population_parallel(state.next_generation, context);

        // Двойная буферизация
        std::swap(state.individuals, state.next_generation);

        // Обновление лучшей особи
        for (const auto& ind : state.individuals) {
            if (ind.fitness < state.best_ever.fitness) {
                state.best_ever = ind;
            }
        }
        state.best_fitness_history.push_back(state.best_ever.fitness);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Материализация финального размещения
    PlacementResult result;
    result.blocks = decoder_.decode_to_layout(state.best_ever, context);
    result.fitness_history = state.best_fitness_history;

    // Расчет метрик качества
    result.metrics.final_cost = state.best_ever.fitness;
    result.metrics.total_hpwl = state.best_ever.raw_hpwl;
    result.metrics.total_area = state.best_ever.raw_area;

    double max_x = 0.0, max_y = 0.0, sum_blocks_area = 0.0;
    for (const auto& b : result.blocks) {
        max_x = std::max(max_x, b.x + b.width);
        max_y = std::max(max_y, b.y + b.height);
        sum_blocks_area += (b.width * b.height);
    }
    result.metrics.actual_width = max_x;
    result.metrics.actual_height = max_y;
    result.metrics.chip_utilization = (result.metrics.total_area > 0.0)
        ? std::min(1.0, sum_blocks_area / result.metrics.total_area) : 0.0;

    // Телеметрия
    result.stats.elapsed_time_ms = elapsed_ms;
    result.stats.total_generations = total_gen;
    result.stats.convergence_generation = compute_convergence_generation(state.best_fitness_history, total_gen);
    result.stats.peak_memory_mb = 8.5; // Базовая оценка, уточняется через WinAPI в main

    return result;
}

void GARunner::initialize_population(
    PopulationState& state,
    const InputContext& context,
    std::mt19937& rng) const {

    const size_t n = context.blocks.size();
    const int pop_size = context.ga_config.population_size;

    state.individuals.resize(static_cast<size_t>(pop_size));
    state.next_generation.resize(static_cast<size_t>(pop_size));

    std::vector<int> base_perm(n);
    std::iota(base_perm.begin(), base_perm.end(), 0);

    for (int i = 0; i < pop_size; ++i) {
        state.individuals[static_cast<size_t>(i)].gamma_plus = base_perm;
        state.individuals[static_cast<size_t>(i)].gamma_minus = base_perm;
        std::shuffle(state.individuals[static_cast<size_t>(i)].gamma_plus.begin(),
                     state.individuals[static_cast<size_t>(i)].gamma_plus.end(), rng);
        std::shuffle(state.individuals[static_cast<size_t>(i)].gamma_minus.begin(),
                     state.individuals[static_cast<size_t>(i)].gamma_minus.end(), rng);

        state.individuals[static_cast<size_t>(i)].is_rotated.assign(n, 0);
        if (context.chip.rotation_allowed) {
            std::uniform_int_distribution<int> bin_dist(0, 1);
            for (size_t b = 0; b < n; ++b) {
                if (context.blocks[b].rotation_allowed) {
                    state.individuals[static_cast<size_t>(i)].is_rotated[b] = static_cast<uint8_t>(bin_dist(rng));
                }
            }
        }

        // Предвыделение буферов следующего поколения для zero allocation
        state.next_generation[static_cast<size_t>(i)].gamma_plus.resize(n);
        state.next_generation[static_cast<size_t>(i)].gamma_minus.resize(n);
        state.next_generation[static_cast<size_t>(i)].is_rotated.resize(n);
    }
}

void GARunner::evaluate_population_parallel(
    std::vector<Individual>& pop,
    const InputContext& context) const {

    const int pop_size = static_cast<int>(pop.size());
    const size_t num_blocks = context.blocks.size();

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        EvaluationScratchpad local_pad;
        local_pad.resize(num_blocks);

#ifdef _OPENMP
        #pragma omp for schedule(static)
#endif
        for (int i = 0; i < pop_size; ++i) {
            decoder_.decode_to_scratchpad(pop[static_cast<size_t>(i)], context, local_pad);
            evaluator_.evaluate(pop[static_cast<size_t>(i)], context, local_pad);
        }
    }
}

int GARunner::tournament_select(
    const std::vector<Individual>& pop,
    int tournament_size,
    std::mt19937& rng) const {

    std::uniform_int_distribution<size_t> dist(0, pop.size() - 1);
    int best_idx = static_cast<int>(dist(rng));
    double best_fit = pop[static_cast<size_t>(best_idx)].fitness;

    for (int i = 1; i < tournament_size; ++i) {
        const int cand_idx = static_cast<int>(dist(rng));
        const double cand_fit = pop[static_cast<size_t>(cand_idx)].fitness;
        if (cand_fit < best_fit) {
            best_fit = cand_fit;
            best_idx = cand_idx;
        }
    }

    return best_idx;
}

void GARunner::order_crossover(
    const std::vector<int>& p1,
    const std::vector<int>& p2,
    std::vector<int>& child,
    std::mt19937& rng) const {

    const size_t n = p1.size();
    if (n <= 1) {
        child = p1;
        return;
    }

    std::uniform_int_distribution<size_t> dist(0, n - 1);
    size_t cut1 = dist(rng);
    size_t cut2 = dist(rng);
    if (cut1 > cut2) {
        std::swap(cut1, cut2);
    }
    if (cut1 == cut2) {
        if (cut2 + 1 < n) cut2++;
        else if (cut1 > 0) cut1--;
    }

    child.assign(n, -1);
    std::vector<bool> in_child(n, false);

    for (size_t i = cut1; i <= cut2; ++i) {
        child[i] = p1[i];
        in_child[static_cast<size_t>(p1[i])] = true;
    }

    size_t child_pos = (cut2 + 1) % n;
    size_t p2_pos = (cut2 + 1) % n;

    for (size_t count = 0; count < n; ++count) {
        const int val = p2[p2_pos];
        if (!in_child[static_cast<size_t>(val)]) {
            child[child_pos] = val;
            in_child[static_cast<size_t>(val)] = true;
            child_pos = (child_pos + 1) % n;
        }
        p2_pos = (p2_pos + 1) % n;
    }
}

void GARunner::pmx_crossover(
    const std::vector<int>& p1,
    const std::vector<int>& p2,
    std::vector<int>& child,
    std::mt19937& rng) const {

    const size_t n = p1.size();
    if (n <= 1) {
        child = p1;
        return;
    }

    std::uniform_int_distribution<size_t> dist(0, n - 1);
    size_t cut1 = dist(rng);
    size_t cut2 = dist(rng);
    if (cut1 > cut2) {
        std::swap(cut1, cut2);
    }
    if (cut1 == cut2) {
        if (cut2 + 1 < n) cut2++;
        else if (cut1 > 0) cut1--;
    }

    child.assign(n, -1);
    std::vector<int> p2_pos(n, -1);
    for (size_t i = 0; i < n; ++i) {
        p2_pos[static_cast<size_t>(p2[i])] = static_cast<int>(i);
    }

    for (size_t i = cut1; i <= cut2; ++i) {
        child[i] = p1[i];
    }

    for (size_t i = cut1; i <= cut2; ++i) {
        const int val = p2[i];
        bool already_in_cut = false;
        for (size_t j = cut1; j <= cut2; ++j) {
            if (p1[j] == val) {
                already_in_cut = true;
                break;
            }
        }
        if (!already_in_cut) {
            int pos = static_cast<int>(i);
            while (pos >= static_cast<int>(cut1) && pos <= static_cast<int>(cut2)) {
                pos = p2_pos[static_cast<size_t>(p1[static_cast<size_t>(pos)])];
            }
            child[static_cast<size_t>(pos)] = val;
        }
    }

    for (size_t i = 0; i < n; ++i) {
        if (child[i] == -1) {
            child[i] = p2[i];
        }
    }
}

void GARunner::mutate_swap(std::vector<int>& perm, std::mt19937& rng) const {
    const size_t n = perm.size();
    if (n < 2) return;
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    size_t i = dist(rng);
    size_t j = dist(rng);
    while (i == j) {
        j = dist(rng);
    }
    std::swap(perm[i], perm[j]);
}

void GARunner::mutate_rotate(
    std::vector<uint8_t>& is_rot,
    const InputContext& context,
    std::mt19937& rng) const {

    if (!context.chip.rotation_allowed || is_rot.empty()) {
        return;
    }
    std::uniform_int_distribution<size_t> dist(0, is_rot.size() - 1);
    const size_t k = dist(rng);
    if (context.blocks[k].rotation_allowed) {
        is_rot[k] = static_cast<uint8_t>(1 - is_rot[k]);
    }
}

int GARunner::compute_convergence_generation(
    const std::vector<double>& history,
    int total_generations) const {

    if (history.empty()) {
        return total_generations;
    }

    const int n_gen = static_cast<int>(history.size());
    const int window = std::min(15, std::max(5, static_cast<int>(0.15 * n_gen)));
    const double eps = 1e-4;

    for (int g = 0; g < n_gen; ++g) {
        const int end_window = std::min(g + window, n_gen - 1);
        bool stable = true;
        const double base_fit = history[static_cast<size_t>(g)];
        const double denom = std::max(1.0, std::abs(base_fit));

        for (int k = g; k <= end_window; ++k) {
            const double diff = std::abs(history[static_cast<size_t>(k)] - base_fit);
            if ((diff / denom) > eps) {
                stable = false;
                break;
            }
        }

        if (stable) {
            return g;
        }
    }

    return total_generations;
}

} // namespace core
