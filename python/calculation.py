"""Запуск C++-задач и чтение табличных результатов."""

import csv
import importlib
import json
import os
from pathlib import Path
import subprocess

import numpy as np

# Реестр расширяется при добавлении новой задачи; имя из JSON не является путём.
TASK_MODULES = {"riemann_problem": "riemann_problem"}


class Calculation:
    def __init__(self, path, task, create=True):
        self.path = Path(path).expanduser().resolve()
        self.task = task
        self.quantities = {}
        if task.type not in TASK_MODULES:
            raise ValueError(f"Неизвестный тип задачи: {task.type}")
        if create:
            self.path.mkdir(parents=True, exist_ok=True)
            config = {"type": task.type, "path": str(self.path), **task.params()}
            with (self.path / "config.json").open("w", encoding="utf-8") as file:
                json.dump(config, file, indent=4, ensure_ascii=False)

    def run(self, bin_dir=None):
        root = Path(__file__).resolve().parents[1]
        if bin_dir is None:
            configured = os.environ.get("GASDYNAMICS_BIN_DIR")
            if configured:
                bin_dir = Path(configured).expanduser()
            else:
                # Presets are preferred; build/bin supports an explicit -B build.
                candidates = [root / "build/release/bin", root / "build/debug/bin",
                              root / "build/bin"]
                bin_dir = next((p for p in candidates if (p / self.task.type).is_file()),
                               candidates[0])
        executable = Path(bin_dir).expanduser().resolve() / self.task.type
        if not executable.is_file():
            raise FileNotFoundError(
                f"Не найдена программа {executable}. Соберите проект или передайте bin_dir.")
        subprocess.run([str(executable), str(self.path / "config.json")], check=True)
        self.find_quantities_csv()
        return self

    def find_quantities_csv(self):
        self.quantities = {}
        # Read older calculations too; new runs write next to config.json.
        data_dir = self.path
        if not (data_dir / "c.csv").is_file() and (data_dir / "results/c.csv").is_file():
            data_dir /= "results"

        def add_columns(file, kind):
            if not file.is_file():
                raise FileNotFoundError(f"Не найден файл результатов: {file}")
            with file.open(encoding="utf-8") as stream:
                columns = next(csv.reader(stream), [])
            if not columns or any(not name.strip() for name in columns):
                raise ValueError(f"Пустой заголовок CSV: {file}")
            for i, name in enumerate(columns):
                name = name.strip()
                if name in self.quantities:
                    raise ValueError(f"Повторяется величина: {name}")
                self.quantities[name] = {"type": kind, "path": file, "column": i}

        for name, kind in [("c.csv", "c"), ("f_x.csv", "f_x"), ("f_t.csv", "f_t")]:
            add_columns(data_dir / name, kind)
        folder = data_dir / "f_xt"
        files = self._layer_files(folder)
        add_columns(files[0], "f_xt")
        for info in self.quantities.values():
            if info["type"] == "f_xt":
                info["path"] = folder

    @staticmethod
    def _layer_files(folder):
        files = list(folder.glob("*.csv"))
        if not files or any(not f.stem.isdecimal() for f in files):
            raise ValueError(f"Не найдены корректные временные слои: {folder}")
        files.sort(key=lambda f: int(f.stem))
        if [int(f.stem) for f in files] != list(range(len(files))):
            raise ValueError(f"Нарушена последовательность временных слоёв: {folder}")
        return files

    @classmethod
    def open(cls, path):
        directory = Path(path).expanduser().resolve()
        with (directory / "config.json").open(encoding="utf-8") as file:
            config = json.load(file)
        task_type = config["type"]
        if task_type not in TASK_MODULES:
            raise ValueError(f"Неизвестный тип задачи: {task_type}")
        config = dict(config)
        mesh = Path(config["mesh"])
        config["mesh"] = str(mesh if mesh.is_absolute() else directory / mesh)
        module = importlib.import_module(TASK_MODULES[task_type])
        calc = cls(directory, module.Problem.open(config), create=False)
        calc.find_quantities_csv()
        return calc

    def load(self, name):
        if name not in self.quantities:
            raise ValueError(f"Величина {name!r} не найдена")
        info = self.quantities[name]
        def read(file):
            return np.loadtxt(file, delimiter=",", skiprows=1,
                              usecols=info["column"], ndmin=1)
        if info["type"] == "f_xt":
            files = self._layer_files(info["path"])
            times = self.load("t")
            coordinates = self.load("x")
            if len(files) != len(times):
                raise ValueError("Количество слоёв не совпадает с числом времён")
            data = np.array([read(file) for file in files])
            if data.shape != (len(times), len(coordinates)):
                raise ValueError("Размеры временных слоёв не соответствуют x и t")
        else:
            data = read(info["path"])
        if not np.isfinite(data).all():
            raise ValueError(f"Нечисловые или бесконечные значения: {name}")
        if name.isidentifier() and name not in dir(self):
            setattr(self, name, data)
        return data
