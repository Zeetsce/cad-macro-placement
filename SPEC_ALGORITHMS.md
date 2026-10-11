# SPEC_ALGORITHMS.md — Спецификация алгоритмов и математических моделей

> **Проект:** САПР топологического проектирования СБИС (Команда 1, группа ЭН-44).  
> **Методы:** Sequence Pair (SP), DAG Longest Path, HPWL, Genetic Algorithm (OX, PMX, Tournament, Elitism).  
> **Инварианты:** Determinism, Zero-Allocation in Evaluation, Quadratic Penalty Confinement.

---

## 1. Математическая модель Sequence Pair (SP)

Представление Sequence Pair (Murata et al., 1995) кодирует взаимное топологическое расположение $N$ непересекающихся прямоугольных макроблоков парой перестановок индексов:
$$S = (\Gamma^+, \Gamma^-), \quad \Gamma^+, \Gamma^- \in \mathfrak{S}_N$$

### 1.1. Правила пространственных отношений
Пусть $\operatorname{pos}^+(i)$ и $\operatorname{pos}^-(i)$ — позиции блока $i$ в перестановках $\Gamma^+$ и $\Gamma^-$ соответственно (индексация от $0$ до $N-1$).

Для любой пары различных блоков $a \neq b$:
1. **Отношение «Левее / Правее» (Horizontal Constraint):**
   $$a \prec_H b \iff \left(\operatorname{pos}^+(a) < \operatorname{pos}^+(b)\right) \land \left(\operatorname{pos}^-(a) < \operatorname{pos}^-(b)\right)$$
   *Физический смысл:* Блок $a$ расположен строго слева от блока $b$.  
   *Ограничение на координаты:* $x_b \ge x_a + w'_a$.

2. **Отношение «Ниже / Выше» (Vertical Constraint):**
   $$b \prec_V a \iff \left(\operatorname{pos}^+(a) < \operatorname{pos}^+(b)\right) \land \left(\operatorname{pos}^-(a) > \operatorname{pos}^-(b)\right)$$
   *Физический смысл:* Блок $b$ расположен строго ниже блока $a$ (блок $a$ выше блока $b$).  
   *Ограничение на координаты:* $y_a \ge y_b + h'_b$.

**Теорема полноты:** Каждая пара блоков $a \neq b$ находится ровно в одном из четырех базовых отношений (слева, справа, снизу, сверху), что математически гарантирует **отсутствие взаимного наложения (no-overlap)** при любом валидном $(\Gamma^+, \Gamma^-)$.

---

### 1.2. Декодирование Sequence Pair в физические координаты

#### Фактические размеры с учетом ориентации
Перед расчетом координат определяются эффективные габариты блоков с учетом вектора `is_rotated`:
$$w'_i = \begin{cases} h_i, & \text{если } \text{is\_rotated}[i] = 1 \\ w_i, & \text{если } \text{is\_rotated}[i] = 0 \end{cases}, \qquad h'_i = \begin{cases} w_i, & \text{если } \text{is\_rotated}[i] = 1 \\ h_i, & \text{если } \text{is\_rotated}[i] = 0 \end{cases}$$

#### Алгоритм прямого декодирования через граф ограничений DAG $O(N^2)$
Для нахождения минимальных координат каждого блока строятся два бесконтурных ориентированных графа ограничений $G_H = (V, E_H)$ и $G_V = (V, E_V)$ и вычисляются длиннейшие пути от фиктивного истока:

1. **Координата $X$ (Горизонтальный длиннейший путь):**
   - Для всех $i \in [0, N-1]$ инициализируем $x[i] = 0.0$.
   - Обходим блоки в порядке перестановки $\Gamma^+$:
     Пусть текущий блок — $b = \Gamma^+[k]$ ($k = 0 \dots N-1$).
     Для каждого предшествующего блока $a = \Gamma^+[j]$ ($j = 0 \dots k-1$):
     Если $\operatorname{pos}^-(a) < \operatorname{pos}^-(b)$, то существует ребро $a \to b \in E_H$:
     $$x[b] = \max(x[b], x[a] + w'_a)$$

2. **Координата $Y$ (Вертикальный длиннейший путь):**
   - Для всех $i \in [0, N-1]$ инициализируем $y[i] = 0.0$.
   - Обходим блоки в порядке перестановки $\Gamma^-$:
     Пусть текущий блок — $a = \Gamma^-[k]$ ($k = 0 \dots N-1$).
     Для каждого предшествующего блока $b = \Gamma^-[j]$ ($j = 0 \dots k-1$):
     Если $\operatorname{pos}^+(b) > \operatorname{pos}^+(a)$, то блок $b$ находится ниже блока $a$ ($b \to a \in E_V$):
     $$y[a] = \max(y[a], y[b] + h'_b)$$

3. **Габариты размещения:**
   $$W_{\text{actual}} = \max_{i=0}^{N-1} (x[i] + w'_i), \qquad H_{\text{actual}} = \max_{i=0}^{N-1} (y[i] + h'_i)$$

4. **Вычисление центров блоков:**
   $$x_{c}[i] = x[i] + \frac{w'_i}{2.0}, \qquad y_{c}[i] = y[i] + \frac{h'_i}{2.0}$$

*Сложность:* Время $O(N^2)$, дополнительная память $O(N)$ (используются плоские векторы в `EvaluationScratchpad`).

---

### 1.3. Эталонный аналитический пример (3 блока)
Данный пример используется в юнит-тесте `Test_Analytical_3_Blocks_No_Rotation` для верификации декодера:
- **Блоки:**
  - $A: w_A = 40, h_A = 20$
  - $B: w_B = 30, h_B = 30$
  - $C: w_C = 20, h_C = 50$
- **Перестановки:**
  - $\Gamma^+ = [A, B, C] \implies \operatorname{pos}^+(A)=0, \operatorname{pos}^+(B)=1, \operatorname{pos}^+(C)=2$
  - $\Gamma^- = [B, A, C] \implies \operatorname{pos}^-(B)=0, \operatorname{pos}^-(A)=1, \operatorname{pos}^-(C)=2$
- **Отношения между парами:**
  - Пара $(A, B)$: $\operatorname{pos}^+(A) < \operatorname{pos}^+(B)$, но $\operatorname{pos}^-(A) > \operatorname{pos}^-(B) \implies B \prec_V A$ ($A$ выше $B$).
  - Пара $(B, C)$: $\operatorname{pos}^+(B) < \operatorname{pos}^+(C)$ и $\operatorname{pos}^-(B) < \operatorname{pos}^-(C) \implies B \prec_H C$ ($C$ правее $B$).
  - Пара $(A, C)$: $\operatorname{pos}^+(A) < \operatorname{pos}^+(C)$ и $\operatorname{pos}^-(A) < \operatorname{pos}^-(C) \implies A \prec_H C$ ($C$ правее $A$).
- **Координаты:**
  - $B$: исток $\implies x_B = 0, y_B = 0$.
  - $A$: выше $B \implies y_A = y_B + h_B = 0 + 30 = 30, x_A = 0$.
  - $C$: правее $B$ ($x_C \ge x_B + w_B = 30$) и правее $A$ ($x_C \ge x_A + w_A = 40$) $\implies x_C = \max(30, 40) = 40, y_C = 0$.
- **Габариты:**
  - $W_{\text{actual}} = \max(0+30, 0+40, 40+20) = 60$.
  - $H_{\text{actual}} = \max(0+30, 30+20, 0+50) = 50$.

---

## 2. Расчет полупериметра связей (HPWL)

Метрика HPWL (Half-Perimeter Wire Length) моделирует суммарную длину межсоединений:
$$\text{HPWL} = \sum_{\text{net} \in \text{nets}} \operatorname{HPWL}(\text{net})$$

Для каждой цепи $\text{net} = \{b_1, b_2, \dots, b_k\}$, соединяющей блоки с центрами $(x_{c, b}, y_{c, b})$:
$$\operatorname{HPWL}(\text{net}) = \left(\max_{b \in \text{net}} x_{c, b} - \min_{b \in \text{net}} x_{c, b}\right) + \left(\max_{b \in \text{net}} y_{c, b} - \min_{b \in \text{net}} y_{c, b}\right)$$

### Краевые условия:
1. Если $\vert\text{net}\vert \le 1$, $\operatorname{HPWL}(\text{net}) = 0.0$.
2. Если `nets` пуст, $\text{HPWL} = 0.0$.
3. Сложность вычисления: $O\left(\sum_{j=1}^M |\text{net}_j|\right)$ — строго линейна по размеру связности.

---

## 3. Целевая функция и штрафы (Cost Function)

### 3.1. Нормализация разномасштабных компонент
Поскольку площадь ($10^6 \dots 10^9$) на несколько порядков превышает длину связей ($10^4 \dots 10^6$), вводится нормализация на константы чипа:
$$\text{Area}_{\text{norm}} = W_{\text{chip}} \times H_{\text{chip}}$$
$$\text{HPWL}_{\text{norm}} = \max\left(1.0, M_{\text{nets}} \times \frac{W_{\text{chip}} + H_{\text{chip}}}{2.0}\right)$$

### 3.2. Барьерный штраф за выход за пределы кристалла
Выход за рамку кристалла делает топологию невалидной (минус 25 баллов). Штрафная функция:
$$\Delta W = \max(0.0, W_{\text{actual}} - W_{\text{chip}}), \qquad \Delta H = \max(0.0, H_{\text{actual}} - H_{\text{chip}})$$
$$\text{Penalty} = C_{\text{penalty}} \cdot \left(\frac{\Delta W}{W_{\text{chip}}} + \frac{\Delta H}{H_{\text{chip}}}\right)^2, \quad \text{где } C_{\text{penalty}} = 50.0$$

### 3.3. Итоговый фитнес-скор (минимизируется):
$$\text{Cost} = w_{\text{hpwl}} \cdot \left(\frac{\text{HPWL}}{\text{HPWL}_{\text{norm}}}\right) + w_{\text{area}} \cdot \left(\frac{\text{Area}}{\text{Area}_{\text{norm}}}\right) + \text{Penalty}$$

---

## 4. Операторы генетического алгоритма (ГА)

### 4.1. Турнирная селекция (`Tournament Selection`)
1. Случайным образом выбираются $K = \text{tournament\_size}$ кандидатов из популяции.
2. Возвращается кандидат с минимальным значением `fitness`.
3. Сложность: $O(K)$.

### 4.2. Оператор Order Crossover (OX)
Применяется раздельно к $\Gamma^+$ и $\Gamma^-$:
1. Выбираются две случайные точки разреза $0 \le cut_1 < cut_2 < N$.
2. Сегмент родителя $P_1[cut_1 \dots cut_2]$ копируется в потомка на те же позиции.
3. Оставшиеся позиции заполняются генами родителя $P_2$, начиная с позиции $cut_2 + 1$ по кругу, пропуская уже присутствующие элементы.
4. Сложность: $O(N)$. Гарантирует валидность перестановки.

### 4.3. Оператор Partially Mapped Crossover (PMX)
1. Выбираются точки разреза $cut_1, cut_2$.
2. Сегмент $P_1[cut_1 \dots cut_2]$ копируется в потомка.
3. Для каждого элемента $P_2[k]$ ($k \in [cut_1 \dots cut_2]$): если его нет в сегменте потомка, находится позиция через отображение $P_2[k] \leftrightarrow P_1[k]$.
4. Оставшиеся незаполненные позиции переносятся из $P_2$.
5. Сложность: $O(N)$. Гарантирует валидность перестановки.

### 4.4. Мутации
1. **Swap-мутация:** С вероятностью $p_{\text{mut}}$ выбираются два случайных индекса $i \neq j$ в $\Gamma^+$ (или $\Gamma^-$) и меняются местами за $O(1)$.
2. **Rotate-мутация:** С вероятностью $p_{\text{mut}}$ выбирается случайный блок $k$. Если `chip.rotation_allowed && blocks[k].rotation_allowed`, то:
   $$\text{is\_rotated}[k] = 1 - \text{is\_rotated}[k]$$
   Иначе мутация игнорируется (no-op).

### 4.5. Стратегия элитизма (`Elitism`)
Перед генерацией потомков $E = \text{elit\_count}$ лучших особей текущего поколения копируются в новое поколение без изменений.
*Инвариант монотонности:*
$$\min_{ind \in P_{t+1}} \text{fitness} \le \min_{ind \in P_t} \text{fitness}$$
