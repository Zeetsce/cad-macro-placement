#!/usr/bin/env python3
"""
generate_test_cases.py — Генератор синтетических тестовых наборов для САПР топологического проектирования.

Категории по ТЗ:
- Легкий: 10-20 блоков, 5-10 цепей.
- Средний: 100-200 блоков, 50-100 цепей.
- Сложный: 500-1000 блоков, 300-500 цепей.
- Граничный: 1000 блоков, плотность заполнения кристалла > 80%.
"""

import json
import os
import random
import sys


def generate_case(
    num_blocks: int,
    num_nets: int,
    chip_w: float,
    chip_h: float,
    grid_size: float = 10.0,
    rotation_allowed: bool = True,
    density_target: float = None,
    seed: int = 42,
) -> dict:
    random.seed(seed)

    blocks = []
    if density_target is not None:
        # Рассчитываем средний размер блока для достижения целевой плотности
        target_total_area = chip_w * chip_h * density_target
        avg_block_area = target_total_area / num_blocks
        avg_side = avg_block_area**0.5
        for i in range(num_blocks):
            w = round(random.uniform(avg_side * 0.8, avg_side * 1.2) / grid_size) * grid_size
            h = round(random.uniform(avg_side * 0.8, avg_side * 1.2) / grid_size) * grid_size
            w = max(grid_size, w)
            h = max(grid_size, h)
            blocks.append({
                "id": f"Cell_{i}",
                "width": float(w),
                "height": float(h),
                "rotation_allowed": rotation_allowed and (random.random() > 0.3),
                "weight": round(random.uniform(0.5, 2.0), 2),
            })
    else:
        for i in range(num_blocks):
            w = float(random.randint(2, 15) * int(grid_size))
            h = float(random.randint(2, 15) * int(grid_size))
            blocks.append({
                "id": f"Cell_{i}",
                "width": w,
                "height": h,
                "rotation_allowed": rotation_allowed and (random.random() > 0.2),
                "weight": round(random.uniform(0.8, 1.5), 2),
            })

    # Генерация цепей (гиперребер)
    nets = []
    for j in range(num_nets):
        degree = random.choice([2, 2, 2, 3, 3, 4, 5])
        connected = random.sample([b["id"] for b in blocks], min(degree, num_blocks))
        nets.append({
            "net_id": f"Net_{j}",
            "block_ids": connected,
        })

    return {
        "design": {
            "chip_width": float(chip_w),
            "chip_height": float(chip_h),
            "grid_size": float(grid_size),
            "rotation_allowed": rotation_allowed,
        },
        "blocks": blocks,
        "nets": nets,
        "ga_params": {
            "population_size": 50 if num_blocks <= 100 else 100,
            "generations": 100 if num_blocks <= 100 else 200,
            "crossover_type": "sequence_pair",
            "selection_method": "tournament",
            "tournament_size": 3,
            "crossover_probability": 0.8,
            "mutation_probability": 0.15,
            "elit_count": 2,
            "fitness_weights": {
                "hpwl": 0.7,
                "area": 0.3,
            },
        },
    }


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "data"
    os.makedirs(out_dir, exist_ok=True)

    test_configs = [
        ("test_easy_15.json", 15, 8, 500.0, 500.0, 10.0, True, None, 101),
        ("test_medium_150.json", 150, 80, 2000.0, 2000.0, 10.0, True, None, 102),
        ("test_hard_600.json", 600, 350, 4500.0, 4500.0, 10.0, True, None, 103),
        ("test_dense_1000.json", 1000, 500, 5000.0, 5000.0, 10.0, True, 0.82, 104),
    ]

    for fname, num_b, num_n, cw, ch, grid, rot, density, seed in test_configs:
        data = generate_case(num_b, num_n, cw, ch, grid, rot, density, seed)
        path = os.path.join(out_dir, fname)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        print(f"[OK] Generated {fname}: {num_b} blocks, {num_n} nets -> {path}")


if __name__ == "__main__":
    main()
