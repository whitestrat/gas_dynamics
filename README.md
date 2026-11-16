# GasDynamics — prac15, задание 1

Точный решатель одномерной задачи Римана для идеального газа. Проект работает
в Linux. Python записывает параметры в JSON и запускает C++-задачу. Задача
обращается к независимому решателю и сохраняет CSV, по которым Python строит графики.

## Подготовка Ubuntu

Нужны компилятор C++17, CMake ≥ 3.21, Ninja, Python ≥ 3.9, NumPy и Matplotlib.
Для Ubuntu 22.04 и новее:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build python3 python3-venv python3-pip
```

Дальнейшие команды выполняются из корня распакованного каталога `gas_dynamics`:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

В новом терминале снова активируйте окружение. JSON-парсер nlohmann/json 3.11.3
включён в `third_party/nlohmann` вместе с лицензией MIT. Во время сборки
скачивать зависимости не требуется.

## Сборка

```bash
cmake --preset release -DPython3_EXECUTABLE="$VIRTUAL_ENV/bin/python"
cmake --build --preset release
```

Исполняемый файл: `build/release/bin/riemann_problem`.
Для отладочной сборки замените `release` на `debug` в обеих командах.
После изменения исходников достаточно повторить команду сборки.

## Пять тестов Торо и результаты задания

```bash
ctest --preset release
cmake --build build/release --target plots
```

CTest запускает один набор `riemann.toro`, содержащий ровно пять обязательных
случаев Торо. Сравниваются `p_star`, `u_star`, `rho_star_left`, `rho_star_right`
с табличными значениями; ошибка даёт ненулевой код завершения.
`ctest --preset release -V` показывает результаты всех пяти проверок.
CSV проверки находятся в `build/release/Testing/toro`.

Цель `plots` сначала рассчитывает эти пять задач в `results/toro`, затем сохраняет:

- `results/toro/comparison.csv` — значения и эталоны, абсолютные и относительные
  ошибки, допуски, типы/скорости волн, оценку GUESSP и число итераций Ньютона;
- `results/toro/test_1` … `test_5` — конфигурации и CSV расчётов;
- `results/toro/test_N/plots` — восемь отдельных рисунков: профили плотности,
  скорости, давления, внутренней энергии, скорости звука, энтропийной функции,
  диаграммы `xt.png` и `pu.png`.

На профиль приходится 1001 точка, сохраняется 101 временной слой. На x–t показаны
поле плотности, ударные волны, контакт, границы и пять внутренних лучей каждого
веера. На p–u сплошные ветви соответствуют ударным волнам, штриховые — разрежению;
в тестах 3–5 давление имеет логарифмическую шкалу.
`entropy` — функция `s=p/rho^gamma`, а не размерная энтропия.

Запуск пяти тестов напрямую:

```bash
python scripts/run_toro.py --bin-dir build/release/bin --output results/toro
```

Входные состояния, моменты вывода и округлённые эталоны находятся в
`examples/riemann/toro_cases.csv`. Абсолютные допуски учитывают округление эталонов.
Для нулевого эталона относительная ошибка не определена: ячейка пуста, проверяется
абсолютный допуск. Сам Ньютон останавливается при относительном изменении давления
меньше `1e-6`; предел — 100 итераций.

## Отдельный расчёт

Подготовленная конфигурация использует относительный путь к соседней сетке:

```bash
./build/release/bin/riemann_problem examples/riemann/config.json
python scripts/plot_results.py examples/riemann
```

CSV записываются рядом с переданным `config.json`. Повторный запуск обновляет
результаты этого расчёта. Рисунки сохраняются в его подкаталоге `plots`.
Графическое окружение рабочего стола не требуется.

Параметры можно задавать из Python через существующие классы проекта:

```python
import sys
from pathlib import Path
sys.path.insert(0, str(Path("python").resolve()))
from calculation import Calculation
from riemann_problem import RiemannProblem

task = RiemannProblem(
    mesh="examples/riemann/mesh.csv",
    states={
        "left": {"density": 1.0, "velocity": 0.0, "pressure": 1.0},
        "right": {"density": 0.125, "velocity": 0.0, "pressure": 0.1},
    },
    time={"t_end": 0.25, "N": 101},
)
calc = Calculation("results/my_case", task)
calc.run(bin_dir="build/release/bin")
print(calc.load("p_star")[0])
```

Затем: `python scripts/plot_results.py results/my_case`.
`Calculation.open("results/my_case")` открывает сохранённый расчёт без повторного
запуска C++; `calc.load("density")` возвращает массив `(N, число точек)`.

JSON содержит `type`, `mesh`, `states`, `time`, `modules`. Поддерживаются
`type="riemann_problem"`, `solver="exact"`, `eos="ideal"`; стандартное `gamma=1.4`.
`time.N` — число слоёв, включая нулевой и последний, `time.t_end` — последний момент.
Сетка — CSV с одной колонкой `x` и не менее 1000 строго возрастающих координат.
Разрыв расположен посередине между крайними координатами сетки.
Относительный путь `mesh` считается от каталога конфигурации. Python сохраняет
`path` как метаданные; каталог вывода определяется расположением JSON.
Плотности и давления должны быть положительными. Вакуум и несходимость дают
сообщение об ошибке и ненулевой код завершения; решение с вакуумом не строится.

## Структура

Сохранено разделение прежнего проекта на задачи, решатели и Python-обработку.
Заготовки будущих подсистем и дополнительные проверочные программы удалены.

| Путь | Назначение |
|---|---|
| `CMakeLists.txt` | Общая конфигурация, подключение решателя, задачи и CTest |
| `CMakePresets.json` | Сборки и запуск тестов `release` / `debug` |
| `include/riemann_exact.hpp` | Независимый header-only решатель: `solve`, `sample`, параметры и потоки |
| `src/tasks/riemann/main.cpp` | JSON и сетка, вызов решателя, экспорт CSV |
| `src/tasks/riemann/CMakeLists.txt` | Цели `riemann_problem`, `export_toro`, `plots` |
| `python/calculation.py` | Запись конфигурации, выбор/запуск C++-задачи, чтение CSV |
| `python/riemann_problem.py` | Параметры задачи Римана |
| `python/graph/` | Общий стиль, профили и диаграммы x–t / p–u |
| `scripts/run_toro.py` | Пять тестов и сводная таблица |
| `scripts/plot_results.py` | Сохранение восьми графиков каждого расчёта |
| `tests/CMakeLists.txt` | Регистрация пяти случаев как одного набора CTest |
| `examples/riemann/` | Рабочие `config.json`, `mesh.csv`, таблица пяти тестов |
| `third_party/nlohmann/` | Используемый JSON-парсер и лицензия |
| `results/` | Создаваемые расчёты; исходно только `.gitkeep` |
| `requirements.txt` | Зависимости Python |
| `.gitignore` | Исключения для сборки, результатов, локального окружения |

Родительские каталоги `src/tools`, `scripts/cluster`, `docs` сохранены пустыми;
используемых файлов для них в задании 1 нет. Новая задача получает подкаталог в
`src/tasks`, подключение в общем CMake, Python-класс и запись в `TASK_MODULES`.
Решатели подключаются к задачам через CMake-цели; файлового ввода внутри решателя нет.

| Результат | Содержимое |
|---|---|
| `c.csv` | Одна строка констант: звёздные состояния, GUESSP, скорости волн, состояние и потоки при `S=0` |
| `f_x.csv` | Координаты `x` и независимо параметризованные давлением кривые p–u |
| `f_t.csv` | Времена и координаты фронтов/лучей |
| `f_xt/0000.csv`, … | Шесть физических величин в узлах сетки каждого слоя |

В `c.csv`: `guess_type` равен 0 для `linear`, 1 для `two_rarefactions`, 2 для
`two_shocks`; `left_is_shock` / `right_is_shock` равны 1 для ударной волны и 0 для
разрежения. Записываются только скорости существующих волн. Потоки `mass_flux`,
`momentum_flux`, `energy_flux` вычисляются как `F(W(S=0))`.

---

# English

## GasDynamics — prac15, task 1

Exact one-dimensional Riemann solver for an ideal gas, intended for Linux.
Python writes JSON parameters and launches a C++ task. The task calls the independent
solver and saves CSV; Python plots those CSV files.

## Ubuntu setup

Requirements: a C++17 compiler, CMake ≥ 3.21, Ninja, Python ≥ 3.9, NumPy and Matplotlib.
On Ubuntu 22.04 or newer:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build python3 python3-venv python3-pip
```

Run subsequent commands from the extracted `gas_dynamics` project root:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

Activate the environment again in each new terminal. nlohmann/json 3.11.3 is bundled
in `third_party/nlohmann` with its MIT license. Building needs no dependency downloads.

## Build

```bash
cmake --preset release -DPython3_EXECUTABLE="$VIRTUAL_ENV/bin/python"
cmake --build --preset release
```

Executable: `build/release/bin/riemann_problem`. Replace `release` with `debug` in both
commands for a debug build. After editing sources, repeat the build command.

## Five Toro tests and required results

```bash
ctest --preset release
cmake --build build/release --target plots
```

CTest runs one suite, `riemann.toro`, containing exactly the five required Toro cases.
It compares `p_star`, `u_star`, `rho_star_left`, `rho_star_right` against reference values
and exits with a nonzero status on failure. `ctest --preset release -V` shows all five
checks. Their CSV output is under `build/release/Testing/toro`.

The `plots` target first computes these cases in `results/toro`, then saves:

- `results/toro/comparison.csv`: computed/reference values, absolute and relative errors,
  tolerances, wave types/speeds, GUESSP initialization and Newton iterations;
- `results/toro/test_1` … `test_5`: configurations and calculation CSV;
- `results/toro/test_N/plots`: eight separate figures — density, velocity, pressure,
  internal energy, sound speed and entropy-function profiles, `xt.png` and `pu.png`.

Each profile uses 1001 points; 101 time layers are saved. The x–t diagram contains the
density field, shocks, contact, fan boundaries and five interior rays per fan. In p–u,
solid branches denote shocks and dashed branches denote rarefactions; tests 3–5 use a
logarithmic pressure axis. `entropy` denotes `s=p/rho^gamma`, not dimensional entropy.

Run the five tests directly:

```bash
python scripts/run_toro.py --bin-dir build/release/bin --output results/toro
```

`examples/riemann/toro_cases.csv` stores input states, output times and rounded reference
values. Absolute tolerances account for reference rounding. Relative error is undefined
for a zero reference: the cell is empty and the absolute tolerance is checked. Newton
itself stops when relative pressure change is below `1e-6`, with at most 100 iterations.

## Individual calculation

The supplied configuration uses a relative path to the adjacent mesh:

```bash
./build/release/bin/riemann_problem examples/riemann/config.json
python scripts/plot_results.py examples/riemann
```

CSV is written next to the specified `config.json`. Rerunning updates that calculation's
results. Figures are saved in its `plots` subdirectory; a graphical desktop is not needed.

Set parameters from Python using the existing project classes:

```python
import sys
from pathlib import Path
sys.path.insert(0, str(Path("python").resolve()))
from calculation import Calculation
from riemann_problem import RiemannProblem

task = RiemannProblem(
    mesh="examples/riemann/mesh.csv",
    states={
        "left": {"density": 1.0, "velocity": 0.0, "pressure": 1.0},
        "right": {"density": 0.125, "velocity": 0.0, "pressure": 0.1},
    },
    time={"t_end": 0.25, "N": 101},
)
calc = Calculation("results/my_case", task)
calc.run(bin_dir="build/release/bin")
print(calc.load("p_star")[0])
```

Then run `python scripts/plot_results.py results/my_case`.
`Calculation.open("results/my_case")` loads a saved calculation without rerunning C++;
`calc.load("density")` returns an array with shape `(N, number_of_points)`.

JSON fields are `type`, `mesh`, `states`, `time`, `modules`. Supported selections are
`type="riemann_problem"`, `solver="exact"`, `eos="ideal"`; the standard `gamma` is 1.4.
`time.N` counts layers including the initial and final ones; `time.t_end` is the final time.
The mesh is a single-column CSV headed `x`, with at least 1000 strictly increasing
coordinates. The initial discontinuity is midway between the mesh endpoints. Relative
mesh paths are resolved against the configuration directory. Python stores `path` as
metadata; the JSON location determines the output directory. Densities and pressures
must be positive. Vacuum and nonconvergence produce an error and nonzero exit status;
vacuum solutions are not constructed.

## Structure

The previous separation into tasks, solvers and Python processing is preserved. Future
subsystem placeholders and additional verification programs have been removed.

| Path | Purpose |
|---|---|
| `CMakeLists.txt` | Overall configuration, solver/task linkage and CTest |
| `CMakePresets.json` | `release` / `debug` build and test presets |
| `include/riemann_exact.hpp` | Independent header-only solver: `solve`, `sample`, properties and fluxes |
| `src/tasks/riemann/main.cpp` | Read JSON/mesh, call solver, export CSV |
| `src/tasks/riemann/CMakeLists.txt` | `riemann_problem`, `export_toro`, `plots` targets |
| `python/calculation.py` | Write configuration, select/run C++ task, load CSV |
| `python/riemann_problem.py` | Riemann problem parameters |
| `python/graph/` | Shared style, profiles, x–t and p–u diagrams |
| `scripts/run_toro.py` | Run the five tests and write the comparison table |
| `scripts/plot_results.py` | Save eight figures per calculation |
| `tests/CMakeLists.txt` | Register the five cases as one CTest suite |
| `examples/riemann/` | Working configuration, mesh and five-case table |
| `third_party/nlohmann/` | The JSON parser used by the task and its license |
| `results/` | Generated calculations; initially only `.gitkeep` |
| `requirements.txt` | Python dependencies |
| `.gitignore` | Exclude build output, results and local environment |

The parent directories `src/tools`, `scripts/cluster`, `docs` remain empty: task 1 has
no active files for them. A new task gets a subdirectory in `src/tasks`, a connection in
the root CMake file, a Python class and an entry in `TASK_MODULES`. Solvers are linked
to tasks through CMake targets; solver code has no file I/O.

| Output | Contents |
|---|---|
| `c.csv` | One row of constants: star states, GUESSP, wave speeds, state and fluxes at `S=0` |
| `f_x.csv` | Mesh coordinates `x` and independently pressure-parametrized p–u curves |
| `f_t.csv` | Times and front/ray coordinates |
| `f_xt/0000.csv`, … | Six physical quantities at mesh points for each time layer |

In `c.csv`, `guess_type` is 0 for `linear`, 1 for `two_rarefactions`, 2 for `two_shocks`.
`left_is_shock` / `right_is_shock` are 1 for a shock and 0 for a rarefaction. Only speeds
of existing waves are exported. `mass_flux`, `momentum_flux`, `energy_flux` are computed
as `F(W(S=0))`.
