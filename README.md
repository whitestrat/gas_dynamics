Программа решает одномерную задачу Римана для идеального газа. Параметры задаются
в JSON. Расчёт выполняется на C++, результаты сохраняются в CSV, графики строятся
с помощью Python. В комплект входят пять тестов Торо.

## 1. Установка необходимого

Команды ниже рассчитаны на Ubuntu 22.04 и новее. В Windows их нужно выполнять
в терминале Ubuntu через WSL.

Установите инструменты сборки и Python:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build python3 python3-venv python3-pip
```

Первая команда обновляет список пакетов. Вторая устанавливает:

- `build-essential` — компилятор C++ и основные инструменты сборки;
- `cmake` — программу, которая подготавливает сборку;
- `ninja-build` — программу, которая выполняет команды сборки;
- `python3` — Python;
- `python3-venv` и `python3-pip` — средства установки модулей Python в отдельное окружение.

Нужны C++17, CMake не ниже 3.21 и Python не ниже 3.9. Посмотреть версии:

```bash
g++ --version
cmake --version
python3 --version
```

Перейдите в каталог проекта. Например, если он находится в `~/project/gas_dynamics`:

```bash
cd ~/project/gas_dynamics
```

Здесь и далее команды выполняются из каталога, в котором лежит `CMakeLists.txt`.
Создайте окружение Python и установите модули:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

Первая команда создаёт окружение `.venv`, вторая включает его, третья устанавливает
NumPy для работы с массивами и Matplotlib для графиков.

В новом терминале снова перейдите в каталог проекта и выполните:

```bash
source .venv/bin/activate
```

Библиотека для чтения JSON уже находится в `third_party/nlohmann`.
Отдельно устанавливать её не нужно.

## 2. Сборка

Сначала подготовьте сборку, указав Python из созданного окружения:

```bash
cmake --preset release -DPython3_EXECUTABLE="$VIRTUAL_ENV/bin/python"
```

Затем соберите программу:

```bash
cmake --build --preset release --parallel
```

`release` — режим с оптимизацией. `--parallel` позволяет компилировать несколько
файлов одновременно. Готовая программа находится в `build/release/bin/riemann_problem`.

После изменения исходников достаточно повторить вторую команду. Если менялись
настройки CMake или окружение Python, сначала повторите первую.

Для отладки есть отдельный режим `debug`:

```bash
cmake --preset debug -DPython3_EXECUTABLE="$VIRTUAL_ENV/bin/python"
cmake --build --preset debug --parallel
```

Он сохраняет данные для отладчика и создаёт программу в `build/debug/bin`.
Сборки `release` и `debug` хранятся отдельно.

Проверьте решатель на пяти тестах Торо:

```bash
ctest --preset release
```

CTest показывает одну проверку `riemann.toro`, внутри которой выполняются все пять
тестов. Подробный вывод можно посмотреть так:

```bash
ctest --preset release -V
```

Сравниваются давление, скорость и две плотности в промежуточных состояниях.
Результаты проверки находятся в `build/release/Testing/toro`.
Для отладочной сборки замените `release` на `debug`.

Если проверки временно не нужны, их можно отключить при настройке:

```bash
cmake --preset release -DBUILD_TESTING=OFF
```

Для включения используйте ту же команду с `-DBUILD_TESTING=ON`.
Расчёты и построение графиков доступны в обоих случаях.

## 3. Работа с программой

### Пять тестов Торо и все графики

```bash
cmake --build --preset release --target plots
```

Команда собирает программу, рассчитывает пять тестов и строит по восемь графиков
для каждого.
Результаты появляются в `results/toro`:

| Путь | Что находится внутри |
| `comparison.csv` | Расчётные и табличные значения, ошибки, параметры волн и итераций |
| `test_1` … `test_5` | Параметры и результаты каждого теста |
| `test_N/plots` | Графики для соответствующего теста |

Для каждого теста сохраняются профили плотности, скорости, давления, внутренней
энергии, скорости звука и энтропийной функции, а также диаграммы x–t и p–u.

На диаграмме p–u шкала давления становится логарифмической, если отношение
максимального положительного давления кривой к минимальному больше 100.

Если нужны только расчёты и таблицы:

```bash
cmake --build --preset release --target export_toro
```

Если расчёты уже выполнены и нужно только заново построить графики:

```bash
python scripts/plot_results.py results/toro
```

Начальные состояния, моменты вывода и табличные значения пяти тестов находятся
в `examples/riemann/toro_cases.csv`.

### Отдельный расчёт

В `examples/riemann/config.json` находится готовый пример. Запустите его:

```bash
./build/release/bin/riemann_problem examples/riemann/config.json
```

Программа прочитает параметры и запишет результаты рядом с `config.json`.
Для построения графиков выполните:

```bash
python scripts/plot_results.py examples/riemann
```

Изображения появятся в `examples/riemann/plots`. 

Основные параметры в JSON:

| Поле | Значение |
| `states.left`, `states.right` | Начальные состояния слева и справа |
| `density`, `velocity`, `pressure` | Плотность, скорость и давление |
| `time.t_end` | Время окончания расчёта |
| `time.N` | Число сохранённых слоёв, включая начальный и последний |
| `mesh` | Путь к CSV с координатами |
| `modules.eos_params.gamma` | Показатель адиабаты |

Сейчас используются `type="riemann_problem"`, `solver="exact"` и `eos="ideal"`.
Плотности и давления должны быть положительными. Решение с вакуумом пока не
поддерживается: программа сообщит об ошибке.

Сетка содержит колонку `x` и не менее 1000 строго возрастающих координат.
Начальный разрыв расположен посередине между крайними координатами.
Относительный путь к сетке считается от каталога с JSON.

Чтобы сохранить собственную постановку отдельно, скопируйте пример вместе с сеткой:

```bash
mkdir -p results/my_case
cp examples/riemann/config.json examples/riemann/mesh.csv results/my_case/
```

Измените `results/my_case/config.json`, затем запустите:

```bash
./build/release/bin/riemann_problem results/my_case/config.json
python scripts/plot_results.py results/my_case
```

Повторный запуск обновляет результаты в этом каталоге.

### Запуск из Python

Классы `RiemannProblem` и `Calculation` позволяют задавать параметры из Python.
Этот пример выполняется из корня проекта:

```python
import sys
sys.path.insert(0, "python")

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

calc = Calculation("results/python_case", task)
calc.run(bin_dir="build/release/bin")
density = calc.load("density")
```

`density` — массив: строки соответствуют моментам времени, столбцы — точкам сетки.
Готовый расчёт можно прочитать без повторного запуска:

```python
calc = Calculation.open("results/python_case")
pressure = calc.load("pressure")
```

### Файлы результатов

| Файл | Содержимое |
| `config.json` | Параметры расчёта |
| `c.csv` | Промежуточные состояния, скорости волн и потоки при `S=0` |
| `f_x.csv` | Координаты и данные для диаграммы p–u |
| `f_t.csv` | Времена и положения волн |
| `f_xt/0000.csv`, … | Плотность, скорость, давление и другие величины на каждом временном слое |
| `plots/` | Сохранённые изображения |

### Каталоги проекта

| Каталог | Назначение |
| `include/` | Решатели, подключаемые к задачам |
| `src/tasks/` | Расчётные задачи на C++ |
| `python/` | Подготовка параметров, запуск и чтение результатов |
| `python/graph/` | Функции построения графиков |
| `scripts/` | Готовые команды расчёта и обработки |
| `tests/` | Подключение тестов к CTest |
| `examples/` | Примеры параметров, сетка и таблица тестов |
| `third_party/` | Сторонние библиотеки |
| `results/` | Результаты расчётов |
| `build/` | Программы и служебные файлы сборки; создаётся автоматически |
| `.venv/` | Окружение Python; создаётся при установке |

Пустые `src/tools`, `scripts/cluster` и `docs` оставлены для инструментов,
кластерных сценариев и документации, которые будут реализованы в следующих версиях программы.

Корневой `CMakeLists.txt` задаёт общие настройки и подключает задачи.
`CMakePresets.json` хранит режимы `release` и `debug`.

