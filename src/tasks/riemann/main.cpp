// Задача связывает JSON/CSV с независимым математическим решателем.
#include "riemann_exact.hpp"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct Ray { std::string name; double speed; };

std::ofstream output(const fs::path& path) {
    std::ofstream file;
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(path);
    file.imbue(std::locale::classic());
    file << std::setprecision(17);
    return file;
}

double number(const json& value) {
    if (!value.is_number()) throw std::runtime_error("Ожидалось число в JSON.");
    const double result = value.get<double>();
    if (!std::isfinite(result)) throw std::runtime_error("Нужно конечное число.");
    return result;
}

riemann::W state(const json& value) {
    return {number(value.at("density")), number(value.at("velocity")),
            number(value.at("pressure"))};
}

std::vector<double> read_mesh(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Не удалось открыть сетку: " + path.string());
    std::string line;
    if (!std::getline(file, line) || line != "x")
        throw std::runtime_error("Ожидается mesh.csv с заголовком x и одной колонкой.");
    std::vector<double> x;
    while (std::getline(file, line)) {
        std::istringstream row(line);
        row.imbue(std::locale::classic());
        double value;
        if (!(row >> value) || !std::isfinite(value) || !(row >> std::ws).eof())
            throw std::runtime_error("Некорректная координата в mesh.csv.");
        if (!x.empty() && value <= x.back())
            throw std::runtime_error("Координаты сетки должны строго возрастать.");
        x.push_back(value);
    }
    if (x.size() < 1000)
        throw std::runtime_error("Для профилей задания 1 нужно не менее 1000 точек.");
    return x;
}

std::vector<Ray> wave_rays(const riemann::Solution& r) {
    std::vector<Ray> rays;
    auto side = [&](const std::string& name, riemann::WaveType type,
                    double shock, double head, double tail) {
        if (type == riemann::WaveType::shock) {
            rays.push_back({name + "_shock", shock});
        } else {
            rays.push_back({name + "_head", head});
            rays.push_back({name + "_tail", tail});
            for (int i = 1; i <= 5; ++i)
                rays.push_back({name + "_fan_" + std::to_string(i),
                                head + (tail - head) * i / 6.0});
        }
    };
    side("left", r.left, r.S_L, r.S_HL, r.S_TL);
    rays.push_back({"contact", r.u_star});
    side("right", r.right, r.S_R, r.S_HR, r.S_TR);
    return rays;
}

void export_solution(const fs::path& directory, const std::vector<double>& x,
                     double t_end, int layers, const riemann::Solution& r) {
    const double x0 = 0.5 * (x.front() + x.back());
    const auto rays = wave_rays(r);
    {
        auto file = output(directory / "c.csv");
        file << "gamma,x0,p_star,u_star,rho_star_left,rho_star_right,p_initial,p_lin,"
                "newton_iterations,guess_type,damped_steps,left_is_shock,right_is_shock,"
                "relative_change,residual,interface_density,interface_velocity,"
                "interface_pressure,mass_flux,momentum_flux,energy_flux";
        for (const auto& ray : rays) file << ',' << ray.name << "_speed";
        file << '\n' << r.gamma << ',' << x0 << ',' << r.p_star << ',' << r.u_star
             << ',' << r.rho_starL << ',' << r.rho_starR << ',' << r.p_initial
             << ',' << r.p_lin << ',' << r.iterations << ',' << static_cast<int>(r.guess)
             << ',' << r.damped_steps << ',' << (r.left == riemann::WaveType::shock)
             << ',' << (r.right == riemann::WaveType::shock) << ',' << r.relative_change
             << ',' << r.residual << ',' << r.W_0.rho << ',' << r.W_0.u << ',' << r.W_0.p
             << ',' << r.F_0.rho_u << ',' << r.F_0.rho_u2_p << ',' << r.F_0.rhoE_p_u;
        for (const auto& ray : rays) file << ',' << ray.speed;
        file << '\n';
    }
    {
        auto file = output(directory / "f_x.csv");
        file << "x,curve_pressure,curve_u_left,curve_u_right\n";
        const double low = 0.1 * std::min({r.W_L.p, r.W_R.p, r.p_star});
        const double high = 2.0 * std::max({r.W_L.p, r.W_R.p, r.p_star});
        const double log_low = std::log(low), log_high = std::log(high);
        for (std::size_t i = 0; i < x.size(); ++i) {
            // Координата x и параметр волновых кривых p независимы.
            const double p = std::exp(log_low + (log_high - log_low) * i / (x.size() - 1));
            file << x[i] << ',' << p << ',' << r.W_L.u - riemann::f(p, r.W_L, r.gamma).f
                 << ',' << r.W_R.u + riemann::f(p, r.W_R, r.gamma).f << '\n';
        }
    }
    const fs::path folder = directory / "f_xt";
    fs::create_directories(folder);
    // При повторном запуске удаляются только ранее записанные числовые CSV-слои.
    for (const auto& entry : fs::directory_iterator(folder)) {
        const std::string stem = entry.path().stem().string();
        if (entry.is_regular_file() && entry.path().extension() == ".csv" &&
            !stem.empty() && stem.find_first_not_of("0123456789") == std::string::npos)
            fs::remove(entry.path());
    }
    auto times = output(directory / "f_t.csv");
    times << 't';
    for (const auto& ray : rays) times << ",x_" << ray.name;
    times << '\n';
    for (int k = 0; k < layers; ++k) {
        const double t = t_end * k / (layers - 1);
        times << t;
        for (const auto& ray : rays) times << ',' << x0 + ray.speed * t;
        times << '\n';
        std::ostringstream filename;
        filename << std::setw(4) << std::setfill('0') << k << ".csv";
        auto file = output(folder / filename.str());
        file << "density,velocity,pressure,internal_energy,sound_speed,entropy\n";
        for (const double position : x) {
            const auto w = t == 0.0 ? (position <= x0 ? r.W_L : r.W_R)
                                   : riemann::sample(r, (position - x0) / t).value();
            const auto v = riemann::parameters(w, r.gamma);
            if (!riemann::valid(w) || !std::isfinite(v.e) || !std::isfinite(v.a) ||
                !std::isfinite(v.s))
                throw std::runtime_error("Состояние вне числового диапазона при экспорте.");
            file << w.rho << ',' << w.u << ',' << w.p << ',' << v.e << ',' << v.a
                 << ',' << v.s << '\n';
        }
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Запуск: riemann_problem <config.json>");
        const fs::path config_path = fs::absolute(argv[1]);
        std::ifstream input(config_path);
        if (!input) throw std::runtime_error("Не удалось открыть конфигурацию.");
        const json config = json::parse(input);
        if (config.at("type") != "riemann_problem" ||
            config.at("modules").at("solver") != "exact" ||
            config.at("modules").at("eos") != "ideal")
            throw std::runtime_error("Поддерживается riemann_problem / exact / ideal.");
        const double gamma = number(config.at("modules").at("eos_params").at("gamma"));
        const double t_end = number(config.at("time").at("t_end"));
        const auto& count = config.at("time").at("N");
        if (!count.is_number_integer() || count.get<long long>() < 2 ||
            count.get<long long>() > 100000 || t_end <= 0.0)
            throw std::runtime_error("Нужны t_end>0 и целое число слоёв 2<=N<=100000.");
        const int layers = count.get<int>();
        fs::path mesh = config.at("mesh").get<std::string>();
        if (mesh.is_relative()) mesh = config_path.parent_path() / mesh;
        const auto x = read_mesh(mesh);
        const auto r = riemann::solve(state(config.at("states").at("left")),
                                      state(config.at("states").at("right")), gamma);
        if (!r.ok()) throw std::runtime_error(r.message);
        export_solution(config_path.parent_path(), x, t_end, layers, r);
        std::cout << std::setprecision(10) << "p*=" << r.p_star << ", u*=" << r.u_star
                  << "; " << riemann::name(r.left) << " / " << riemann::name(r.right)
                  << "; GUESSP=" << riemann::name(r.guess)
                  << "; iterations=" << r.iterations << '\n'
                  << "CSV: " << config_path.parent_path() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
