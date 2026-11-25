from pathlib import Path

class RiemannProblem:
    """Класс задания параметров задачи о распаде произвольного разрыва"""
    type = "riemann_problem"
    def __init__(self, mesh = "mesh.csv", 
                 states = {"left": {
                            "density": 1.0,
                            "velocity": 0.0,
                            "pressure": 1.0
                          },
                           "right": {
                            "density": 0.125,
                            "velocity": 0.0,
                            "pressure": 0.1
                          }
                        },
                 time = {"t_end": 0.012, "N": 100},
                 modules = {
                     "solver": "exact",
                     "eos": "ideal",
                     "eos_params": {"gamma": 1.4}
                 }):
        mesh = Path(mesh)
        self.mesh = str(mesh.absolute())
        self.states = states
        self.time = time
        self.modules = modules
    #TO_DO Проверка введённых значений
    
    #Загрузка параметров задачи из файла
    @classmethod
    def open(cls, config):
        return cls(mesh=config["mesh"],
                   states=config["states"],
                   time=config["time"],
                   modules=config["modules"])
    #Возвращаем словарь для записи параметров задачи в json
    def params(self):
        return {"mesh": self.mesh, "states": self.states, "time": self.time, "modules": self.modules}
    
Problem = RiemannProblem