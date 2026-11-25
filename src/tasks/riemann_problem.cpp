#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// Временное чтение простого JSON. Позже заменить полноценным JSON-парсером.
std::string object_from_json(const std::string& text, const std::string& key) {
    const std::regex key_pattern("\"" + key + "\"\\s*:\\s*\\{");
    std::smatch match;
    if (!std::regex_search(text, match, key_pattern)) return {};

    const std::size_t start = static_cast<std::size_t>(match.position() + match.length() - 1);
    int depth = 0;
    for (std::size_t i = start; i < text.size(); ++i) {
        if (text[i] == '{') ++depth;
        if (text[i] == '}' && --depth == 0) return text.substr(start, i - start + 1);
    }
    throw std::runtime_error("Незакрытый объект JSON: " + key);
}

double number_from_json(const std::string& text, const std::string& key, double fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*([-+]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? std::stod(match[1].str()) : fallback;
}

std::string string_from_json(const std::string& text, const std::string& key,
                             const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? match[1].str() : fallback;
}

struct State {
    double density;
    double velocity;
    double pressure;
};

State read_state(const std::string& states, const std::string& side) {
    const std::string item = object_from_json(states, side);
    if (item.empty()) throw std::runtime_error("Нет состояния states." + side);

    State w{
        number_from_json(item, "density", -1.0),
        number_from_json(item, "velocity", 0.0),
        number_from_json(item, "pressure", -1.0)
    };
    if (!(w.density > 0.0) || !(w.pressure > 0.0) || !std::isfinite(w.velocity))
        throw std::runtime_error("Некорректные density, velocity, pressure в states." + side);
    return w;
}

std::vector<double> read_mesh(const fs::path& file) {
    std::vector<double> x;
    std::ifstream in(file);
    if (!in) {
        // Заглушка может сама создать демонстрационную сетку.
        std::ofstream out(file);
        if (!out) throw std::runtime_error("Не удалось создать " + file.string());
        out << "x\n" << std::setprecision(17);
        for (int i = 0; i <= 1000; ++i) {
            const double xi = i / 1000.0;
            x.push_back(xi);
            out << xi << '\n';
        }
        return x;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const std::string field = line.substr(0, line.find(','));
        if (field == "x" || field == "x\r") continue;
        try {
            std::size_t used = 0;
            const double xi = std::stod(field, &used);
            if (field.find_first_not_of(" \t\r", used) != std::string::npos || !std::isfinite(xi))
                throw std::runtime_error("Некорректная координата x в " + file.string());
            x.push_back(xi);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Некорректная строка сетки: " + line);
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Координата вне диапазона: " + line);
        }
    }
    if (x.size() < 2 || !std::is_sorted(x.begin(), x.end()) ||
        std::adjacent_find(x.begin(), x.end()) != x.end())
        throw std::runtime_error("Нужны как минимум две строго возрастающие координаты x");
    return x;
}

// Функции волновых кривых используются только для определения типа волн.
// Пространственный профиль внутри веера по-прежнему демонстрационный.
double wave_function(double p, const State& w, double gamma) {
    const double a = std::sqrt(gamma * w.pressure / w.density);
    if (p <= w.pressure) {
        const double exponent = (gamma - 1.0) / (2.0 * gamma);
        return 2.0 * a / (gamma - 1.0) * (std::pow(p / w.pressure, exponent) - 1.0);
    }
    const double A = 2.0 / ((gamma + 1.0) * w.density);
    const double B = (gamma - 1.0) / (gamma + 1.0) * w.pressure;
    return (p - w.pressure) * std::sqrt(A / (p + B));
}

double find_star_pressure(const State& left, const State& right, double gamma) {
    const double aL = std::sqrt(gamma * left.pressure / left.density);
    const double aR = std::sqrt(gamma * right.pressure / right.density);
    if (right.velocity - left.velocity >= 2.0 * (aL + aR) / (gamma - 1.0))
        throw std::runtime_error("Возникает вакуум; заглушка его не обрабатывает");

    const auto residual = [&](double p) {
        return wave_function(p, left, gamma) + wave_function(p, right, gamma)
               + right.velocity - left.velocity;
    };
    double lo = 0.0;
    double hi = std::max(left.pressure, right.pressure);
    while (residual(hi) < 0.0) hi *= 2.0;
    for (int i = 0; i < 100; ++i) {
        const double mid = 0.5 * (lo + hi);
        if (residual(mid) < 0.0) lo = mid;
        else hi = mid;
    }
    return 0.5 * (lo + hi);
}

enum class WaveType { absent, shock, rarefaction };

struct Wave {
    WaveType type = WaveType::absent;
    double shock = 0.0;
    double head = 0.0;
    double tail = 0.0;
};

Wave make_wave(const State& w, double p_star, double u_star, double gamma, bool left) {
    Wave wave;
    if (std::abs(p_star - w.pressure) <= 1e-10 * std::max(p_star, w.pressure))
        return wave;

    const double a = std::sqrt(gamma * w.pressure / w.density);
    if (p_star > w.pressure) {
        wave.type = WaveType::shock;
        const double ratio = p_star / w.pressure;
        const double factor = std::sqrt((gamma + 1.0) / (2.0 * gamma) * ratio
                                        + (gamma - 1.0) / (2.0 * gamma));
        wave.shock = w.velocity + (left ? -a * factor : a * factor);
    } else {
        wave.type = WaveType::rarefaction;
        const double a_star = a * std::pow(p_star / w.pressure,
                                          (gamma - 1.0) / (2.0 * gamma));
        wave.head = w.velocity + (left ? -a : a);
        wave.tail = u_star + (left ? -a_star : a_star);
    }
    return wave;
}

double star_density(const State& w, double p_star, double gamma) {
    const double ratio = p_star / w.pressure;
    if (p_star <= w.pressure) return w.density * std::pow(ratio, 1.0 / gamma);
    const double c = (gamma - 1.0) / (gamma + 1.0);
    return w.density * (ratio + c) / (c * ratio + 1.0);
}

State interpolate(const State& a, const State& b, double q) {
    q = std::clamp(q, 0.0, 1.0);
    return {a.density + (b.density - a.density) * q,
            a.velocity + (b.velocity - a.velocity) * q,
            a.pressure + (b.pressure - a.pressure) * q};
}

// Демонстрационный отбор: на постоянных участках используются звёздные
// состояния, внутри веера — линейная интерполяция вместо точной формулы.
State sample(double x, double t, double x0, const State& left, const State& right,
             const State& star_left, const State& star_right, double u_star,
             const Wave& lw, const Wave& rw) {
    if (t == 0.0) return x < x0 ? left : right;
    const double s = (x - x0) / t;

    if (s < u_star) {
        if (lw.type == WaveType::shock && s < lw.shock) return left;
        if (lw.type == WaveType::rarefaction) {
            if (s < lw.head) return left;
            if (s < lw.tail)
                return interpolate(left, star_left, (s - lw.head) / (lw.tail - lw.head));
        }
        return star_left;
    }

    if (rw.type == WaveType::shock && s > rw.shock) return right;
    if (rw.type == WaveType::rarefaction) {
        if (s > rw.head) return right;
        if (s > rw.tail)
            return interpolate(star_right, right, (s - rw.tail) / (rw.head - rw.tail));
    }
    return star_right;
}

using Front = std::pair<std::string, double>;

void add_fronts(std::vector<Front>& fronts, const Wave& wave, const std::string& side) {
    if (wave.type == WaveType::shock) {
        fronts.emplace_back("x_" + side + "_shock", wave.shock);
    } else if (wave.type == WaveType::rarefaction) {
        fronts.emplace_back("x_" + side + "_head", wave.head);
        fronts.emplace_back("x_" + side + "_tail", wave.tail);
        for (int i = 1; i <= 5; ++i) {
            fronts.emplace_back("x_" + side + "_fan_" + std::to_string(i),
                                wave.head + (wave.tail - wave.head) * i / 6.0);
        }
    }
}

std::ofstream create_csv(const fs::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Не удалось создать " + path.string());
    out << std::setprecision(17);
    return out;
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Использование: riemann_problem <config.json>\n";
            return 1;
        }

        const fs::path config_path = fs::absolute(argv[1]);
        std::ifstream in(config_path);
        if (!in) throw std::runtime_error("Не найден " + config_path.string());
        const std::string json((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
        const fs::path directory = config_path.parent_path();

        const std::string time = object_from_json(json, "time");
        const std::string modules = object_from_json(json, "modules");
        const std::string eos = object_from_json(modules, "eos_params");
        const std::string states = object_from_json(json, "states");
        const State left = read_state(states, "left");
        const State right = read_state(states, "right");

        const double t_end = number_from_json(time, "t_end",
                             number_from_json(json, "final_time", 0.25));
        const double n = number_from_json(time, "N",
                         number_from_json(json, "time_layers", 51));
        const double gamma = number_from_json(eos, "gamma",
                             number_from_json(json, "gamma", 1.4));
        if (!std::isfinite(t_end) || !(t_end > 0.0) || !std::isfinite(n) ||
            n < 2.0 || n > 100000.0 || std::floor(n) != n ||
            !std::isfinite(gamma) || !(gamma > 1.0))
            throw std::runtime_error("Требуется time.t_end > 0, целое time.N >= 2, gamma > 1");
        const int nt = static_cast<int>(n);

        const fs::path mesh_name = string_from_json(json, "mesh", "mesh.csv");
        const fs::path mesh_path = mesh_name.is_absolute() ? mesh_name : directory / mesh_name;
        const std::vector<double> x = read_mesh(mesh_path);
        const double x0 = number_from_json(json, "discontinuity",
                                           0.5 * (x.front() + x.back()));

        const double p_star = find_star_pressure(left, right, gamma);
        const double fL = wave_function(p_star, left, gamma);
        const double fR = wave_function(p_star, right, gamma);
        const double u_star = 0.5 * (left.velocity + right.velocity + fR - fL);
        const State star_left{star_density(left, p_star, gamma), u_star, p_star};
        const State star_right{star_density(right, p_star, gamma), u_star, p_star};
        const Wave lw = make_wave(left, p_star, u_star, gamma, true);
        const Wave rw = make_wave(right, p_star, u_star, gamma, false);

        std::vector<Front> fronts;
        add_fronts(fronts, lw, "left");
        if (std::abs(star_left.density - star_right.density) >
            1e-10 * std::max(star_left.density, star_right.density))
            fronts.emplace_back("x_contact", u_star);
        add_fronts(fronts, rw, "right");

        // Все результаты лежат рядом с config.json, БЕЗ каталога results/.
        const fs::path layers_dir = directory / "f_xt";
        fs::create_directories(layers_dir);
        for (const auto& entry : fs::directory_iterator(layers_dir))
            if (entry.is_regular_file() && entry.path().extension() == ".csv")
                fs::remove(entry.path());

        // Константы.
        {
            auto out = create_csv(directory / "c.csv");
            out << "gamma,x0,p_star,u_star,rho_star_left,rho_star_right\n";
            out << gamma << ',' << x0 << ',' << p_star << ',' << u_star << ','
                << star_left.density << ',' << star_right.density << '\n';
        }

        // Координата x и временные данные для демонстрационного p-u графика.
        // curve_* пока остаются в f_x.csv для совместимости с Python.
        {
            auto out = create_csv(directory / "f_x.csv");
            out << "x,curve_pressure,curve_u_left,curve_u_right\n";
            const double p_min = std::min({left.pressure, right.pressure, p_star}) * 0.1;
            const double p_max = std::max({left.pressure, right.pressure, p_star}) * 3.0;
            for (std::size_t i = 0; i < x.size(); ++i) {
                const double q = static_cast<double>(i) / (x.size() - 1);
                const double p = p_min * std::pow(p_max / p_min, q);
                out << x[i] << ',' << p << ','
                    << left.velocity - wave_function(p, left, gamma) << ','
                    << right.velocity + wave_function(p, right, gamma) << '\n';
            }
        }

        // Время и ТОЛЬКО существующие волны.
        {
            auto out = create_csv(directory / "f_t.csv");
            out << "t";
            for (const auto& front : fronts) out << ',' << front.first;
            out << '\n';
            for (int k = 0; k < nt; ++k) {
                const double t = t_end * k / (nt - 1);
                out << t;
                for (const auto& front : fronts) out << ',' << x0 + front.second * t;
                out << '\n';
            }
        }

        // Поля f_xt: каждая строка соответствует координате из f_x.csv.
        for (int k = 0; k < nt; ++k) {
            const double t = t_end * k / (nt - 1);
            std::ostringstream filename;
            filename << std::setw(4) << std::setfill('0') << k << ".csv";
            auto out = create_csv(layers_dir / filename.str());
            out << "density,velocity,pressure,internal_energy,sound_speed,entropy\n";
            for (double xi : x) {
                const State w = sample(xi, t, x0, left, right, star_left, star_right,
                                       u_star, lw, rw);
                const double e = w.pressure / ((gamma - 1.0) * w.density);
                const double a = std::sqrt(gamma * w.pressure / w.density);
                const double entropy = w.pressure / std::pow(w.density, gamma);
                out << w.density << ',' << w.velocity << ',' << w.pressure << ','
                    << e << ',' << a << ',' << entropy << '\n';
            }
        }

        std::cout << "Заглушка завершена: " << directory << '\n';
        std::cout << "Точек x: " << x.size() << ", временных слоёв: " << nt << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
