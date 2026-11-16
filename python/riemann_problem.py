"""Параметры задачи Римана; физика выполняется в C++."""

from copy import deepcopy
from pathlib import Path


class RiemannProblem:
    type = "riemann_problem"

    def __init__(self, mesh="mesh.csv", states=None, time=None, modules=None):
        self.mesh = str(Path(mesh).expanduser().resolve())
        self.states = deepcopy(states if states is not None else {
            "left": {"density": 1.0, "velocity": 0.0, "pressure": 1.0},
            "right": {"density": 0.125, "velocity": 0.0, "pressure": 0.1}})
        self.time = deepcopy(time if time is not None else {"t_end": 0.1, "N": 100})
        self.modules = deepcopy(modules if modules is not None else {
            "solver": "exact", "eos": "ideal", "eos_params": {"gamma": 1.4}})

    @classmethod
    def open(cls, config):
        return cls(mesh=config["mesh"], states=config["states"],
                   time=config["time"], modules=config["modules"])

    def params(self):
        return deepcopy({"mesh": self.mesh, "states": self.states,
                         "time": self.time, "modules": self.modules})


Problem = RiemannProblem
