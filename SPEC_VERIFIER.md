# SPEC_VERIFIER.md — Спецификация модуля независимой верификации

> **Проект:** САПР топологического проектирования СБИС (Команда 1, группа ЭН-44).  
> **Модуль:** `verifier::PlacementVerifier`.  
> **Критерий:** 25 баллов ТЗ за корректность, защита от штрафа -5 за непройденные тесты.

---

## 1. Архитектурная роль и SRP (Single Responsibility Principle)

`PlacementVerifier` — полностью автономный сервис геометрического и топологического аудита.  
Он моделирует проверочный стенд преподавателя и чекер кросс-ревью:
- **Вход:** Иммутабельный `const core::InputContext&`, массив `const std::vector<core::PlacedBlock>&`, заявленный `double claimed_hpwl`.
- **Выход:** `verifier::VerificationReport` (булев вердикт `is_valid` и детальные списки нарушений).
- **Изоляция:** Модуль ничего не знает о Sequence Pair, генах ГА или фитнес-функциях. Анализируется исключительно чистая геометрия $X, Y, W, H$.
- **Fail-Fast & Zero Modification:** Модуль никогда не пытается «поправить» геометрию или подвинуть блоки; любые дефекты строго протоколируются.

---

## 2. Физика проверок и алгоритмические формулы

### 2.1. Квадратичный аудит наложений (AABB Overlap Audit) $O(N^2)$
Для всех уникальных пар $(i, j)$ таких, что $0 \le i < j < N$:
Два прямоугольника $[x_i, x_i + w_i] \times [y_i, y_i + h_i]$ и $[x_j, x_j + w_j] \times [y_j, y_j + h_j]$ пересекаются тогда и только тогда, когда:
$$\operatorname{Overlap}(i, j) \iff \neg\left( x_i + w_i \le x_j + \varepsilon \;\lor\; x_j + w_j \le x_i + \varepsilon \;\lor\; y_i + h_i \le y_j + \varepsilon \;\lor\; y_j + h_j \le y_i + \varepsilon \right)$$
где $\varepsilon = 10^{-7}$ — численный допуск для исключения погрешностей операций с плавающей точкой (`double`).
- *Касание сторонами:* разрешено ($x_i + w_i = x_j$).
- *Пересечение внутренних площадей:* фиксируется как `OverlapViolation { i, j, intersection_area }`.

---

### 2.2. Проверка выхода за границы кристалла (Boundary Confinement) $O(N)$
Для каждого блока $i \in [0, N-1]$ проверяются 4 геометрических неравенства:
1. $x_i \ge -\varepsilon$
2. $y_i \ge -\varepsilon$
3. $x_i + w_i \le W_{\text{chip}} + \varepsilon$
4. $y_i + h_i \le H_{\text{chip}} + \varepsilon$

При нарушении формируется запись:
$$\text{overflow\_x} = \max(0.0, x_i + w_i - W_{\text{chip}}) + \max(0.0, -x_i)$$
$$\text{overflow\_y} = \max(0.0, y_i + h_i - H_{\text{chip}}) + \max(0.0, -y_i)$$
Заносится в `report.boundary_escapes`.

---

### 2.3. Выравнивание по координатной сетке (Grid Alignment) $O(N)$
По ТЗ координаты левого нижнего угла должны быть кратны шагу сетки `grid_size`:
$$\operatorname{fmod}(x_i, \text{grid\_size}) < \varepsilon \;\lor\; \operatorname{fmod}(x_i, \text{grid\_size}) > \text{grid\_size} - \varepsilon$$
$$\operatorname{fmod}(y_i, \text{grid\_size}) < \varepsilon \;\lor\; \operatorname{fmod}(y_i, \text{grid\_size}) > \text{grid\_size} - \varepsilon$$
При нарушении индекс блока добавляется в `report.grid_violations`.

---

### 2.4. Аудит санкционированности поворота (Rotation Legality) $O(N)$
Если у блока $w_i \neq w_i^{\text{nominal}}$ (блок повёрнут на $90^\circ$):
Блок считается повернутым нелегально, если:
$$\neg\text{context.chip.rotation\_allowed} \;\lor\; \neg\text{context.blocks}[i]\text{.rotation\_allowed}$$
При нарушении индекс добавляется в `report.illegal_rotations`.

---

### 2.5. Контроль полноты и биективности (Completeness & Bijectivity) $O(N)$
- Проверяется, что размер массива `blocks.size() == context.blocks.size()`.
- Проверяется массив присутствия `visited[N]`. Каждый `block_id` обязан встретиться ровно один раз.
- Все пропущенные или продублированные индексы фиксируются в `report.missing_or_duplicate_blocks`.

---

### 2.6. Независимый контрольный перерасчет HPWL $O\left(\sum |\text{net}|\right)$
Верификатор рассчитывает независимый полупериметр $HPWL_{\text{audit}}$ по формуле геометрических центров:
$$x_{c, i} = x_i + \frac{w_i}{2.0}, \qquad y_{c, i} = y_i + \frac{h_i}{2.0}$$
$$HPWL_{\text{audit}} = \sum_{\text{net} \in \text{nets}} \left[\left(\max_{k \in \text{net}} x_{c, k} - \min_{k \in \text{net}} x_{c, k}\right) + \left(\max_{k \in \text{net}} y_{c, k} - \min_{k \in \text{net}} y_{c, k}\right)\right]$$
Проверка согласованности:
$$|HPWL_{\text{audit}} - claimed\_hpwl| < 10^{-5}$$
Флаг `report.hpwl_matches` выставляется в `true` только при совпадении.

---

## 3. C++17 Интерфейс: `include/verifier/PlacementVerifier.hpp`

```cpp
#pragma once

#include "core/InputContext.hpp"
#include "core/PlacedBlock.hpp"
#include "verifier/VerificationReport.hpp"

#include <vector>

namespace verifier {

class PlacementVerifier {
public:
    PlacementVerifier() = default;
    ~PlacementVerifier() = default;

    /**
     * @brief Полный независимый аудит топологии.
     */
    VerificationReport verify(
        const core::InputContext& context,
        const std::vector<core::PlacedBlock>& blocks,
        double claimed_hpwl) const;

    /**
     * @brief Экспресс-проверка геометрии (для быстрых ассертов в тестах).
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
```

---

## 4. Спецификация тестового набора (`tests/test_verifier.cpp`)

| Имя теста | Сценарий проверки | Ожидаемый вердикт |
|---|---|---|
| `Test_Verifier_Valid_Layout` | 2 блока рядом: $A(0, 0, 100, 100)$, $B(100, 0, 100, 100)$ на чипе $1000 \times 1000$. | `is_valid == true`, все списки дефектов пусты. |
| `Test_Verifier_Catches_Exact_Overlap` | Блоки с наложением: $A(0, 0, 50, 50)$, $B(40, 40, 50, 50)$. | `is_valid == false`, `overlaps.size() == 1`, $A$ и $B$ зафиксированы. |
| `Test_Verifier_Catches_Boundary_Escape` | Блок $A(980, 0, 50, 50)$ на чипе $1000 \times 1000$. | `is_valid == false`, `boundary_escapes.size() == 1`, `overflow_x == 30.0`. |
| `Test_Verifier_Detects_Illegal_Rotation` | Блок с $w=20, h=40$ при номинале $40 \times 20$, но `rotation_allowed == false`. | `is_valid == false`, блок в `illegal_rotations`. |
| `Test_Verifier_Detects_Grid_Violation` | Координата $x = 15.0$ при `grid_size = 10.0`. | `is_valid == false`, блок в `grid_violations`. |
| `Test_Verifier_Catches_HPWL_Mismatch` | Передача `claimed_hpwl = 100.0` при фактическом $150.0$. | `hpwl_matches == false`, `is_valid == false`. |
