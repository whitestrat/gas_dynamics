import json
import csv
from pathlib import Path
import importlib
import numpy as np
import subprocess

class Calculation:
    """Клас для запуска и обработки задачи в общем случае"""
    def __init__(self, path, task, create=True):
        #Создаём директорию для записи
        self.path = Path(path)
        self.task = task
        self.quantities = {}
        if(create):    
            self.path.mkdir(parents=True, exist_ok=True)
            #Создаём словарь для записи в json
            config = {"type": task.type, "path": str(self.path.absolute())}
            params = task.params()
            for key in params.keys():
                config[key] = params[key]
            with (self.path / "config.json").open("w", encoding="utf-8") as file:
                json.dump(config, file, indent=4, ensure_ascii=False)
    
    def run(self):
        root = Path(__file__).resolve().parents[1]
        executable = root / "build" / "bin" / self.task.type
        config = (self.path / "config.json").resolve()
        subprocess.run([str(executable), str(config)], check=True)
        self.find_quantities_csv()
    
    #Находим список величин по файлам в директории
    def find_quantities_csv(self):
        self.quantities = {}
        def add_columns(file, kind):
            if not file.is_file():
                return
            with file.open("r", encoding="utf-8") as f:
                columns = next(csv.reader(f), [])
            for i, name in enumerate(columns):
                name = name.strip()
                if not name:
                    continue
                if name in self.quantities:
                    raise ValueError(f"Повторяется величина: {name}")
                self.quantities[name] = {
                    "type": kind,
                    "path": file,
                    "column": i
                }
        add_columns(self.path / "c.csv", "c")
        add_columns(self.path / "f_x.csv", "f_x")
        add_columns(self.path / "f_t.csv", "f_t")
        folder = self.path / "f_xt"
        files = sorted(folder.glob("*.csv"))
        if files:
            add_columns(files[0], "f_xt")
            for info in self.quantities.values():
                if info["type"] == "f_xt":
                    info["path"] = folder
       
    #Чтение из готового расположения
    @classmethod
    def open(cls, path):
        path = Path(path) / "config.json"
        with path.open("r", encoding="utf-8") as file:
            config = json.load(file)
        module = importlib.import_module(config["type"])
        task = module.Problem.open(config)
        calc =  cls(path, task, create=False)
        calc.find_quantities_csv()

    def load(self, name):
        if name not in self.quantities:
            raise ValueError(f"Величина '{name}' не найдена")

        info = self.quantities[name]
        path = info["path"]
        col = info["column"]

        if info["type"] == "f_xt":
            files = sorted(path.glob("*.csv"), key=lambda f: int(f.stem))
            data = np.array([np.loadtxt(f, delimiter=",", skiprows=1,
                                        usecols=col, ndmin=1) for f in files])
        else:
            data = np.loadtxt(path, delimiter=",", skiprows=1,
                            usecols=col, ndmin=1)

        if name.isidentifier() and name not in dir(self):
            setattr(self, name, data)

        return data


    